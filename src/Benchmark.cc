#include "Benchmark.hh"

#include "GpuSimulation.hh"
#include "Scenario.hh"
#include "Simulation.hh"

#include <DirectXMath.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <omp.h>
#include <print>
#include <vector>

namespace
{

    constexpr float step_time { 1.0f / 480.0f }; // the physics step used everywhere

    // milliseconds between two moments
    double elapsed_ms(std::chrono::steady_clock::time_point start)
    {

        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

    }

    // average milliseconds per step for the GPU simulation (waits for the GPU, since dispatches return immediately)
    double time_gpu(const Graphics& graphics, GpuSimulation& simulation, int warmup_steps, int timed_steps)
    {

        for (int s { 0 }; s < warmup_steps; ++s)
        {

            simulation.step(graphics, step_time);

        }

        graphics.wait_for_gpu();
        const auto start { std::chrono::steady_clock::now() };

        for (int s { 0 }; s < timed_steps; ++s)
        {

            simulation.step(graphics, step_time);

        }

        graphics.wait_for_gpu();
        return elapsed_ms(start) / static_cast<double>(timed_steps);

    }

    // average milliseconds per step for the CPU simulation
    double time_cpu(Simulation& simulation, int warmup_steps, int timed_steps)
    {

        for (int s { 0 }; s < warmup_steps; ++s)
        {

            simulation.step(step_time);

        }

        const auto start { std::chrono::steady_clock::now() };

        for (int s { 0 }; s < timed_steps; ++s)
        {

            simulation.step(step_time);

        }

        return elapsed_ms(start) / static_cast<double>(timed_steps);

    }

}

void run_benchmark(const Graphics& graphics)
{

    // part 1: run the CPU and GPU simulations from the same start and compare them
    {

        const Scenario scenario { make_scenario(10) };
        Simulation cpu { scenario.points, scenario.bounds_min, scenario.bounds_max };
        GpuSimulation gpu { graphics, scenario.points, scenario.bounds_min, scenario.bounds_max };

        // after one step, both have computed densities from the same positions
        cpu.step(step_time);
        gpu.step(graphics, step_time);
        const std::vector<float> gpu_densities { gpu.read_densities(graphics) };
        float max_density_difference { 0.0f };

        for (std::size_t i { 0uz }; i < gpu_densities.size(); ++i)
        {

            max_density_difference = std::max(max_density_difference, std::abs(gpu_densities[i] - cpu.densities()[i]) / std::max(1.0f, cpu.densities()[i]));

        }

        // after 100 more steps, compare where the particles ended up
        for (int s { 0 }; s < 100; ++s)
        {

            cpu.step(step_time);
            gpu.step(graphics, step_time);

        }

        const std::vector<::DirectX::XMFLOAT4> gpu_positions { gpu.read_positions(graphics) };
        float max_position_difference { 0.0f };

        for (std::size_t i { 0uz }; i < gpu_positions.size(); ++i)
        {

            max_position_difference = std::max({ max_position_difference, std::abs(gpu_positions[i].x - cpu.positions()[i].x), std::abs(gpu_positions[i].y - cpu.positions()[i].y), std::abs(gpu_positions[i].z - cpu.positions()[i].z) });

        }

        std::println("validation, {} particles: max relative density difference after 1 step = {:.2e}, max position difference after 101 steps = {:.2e} m", gpu_positions.size(), max_density_difference, max_position_difference);

    }

    // part 2: time both at increasing particle counts, starting from a cube at rest on the floor
    std::println("");
    std::println("{} CPU threads (OpenMP), physics step = 1/480 s, simulation only (no drawing)", omp_get_max_threads());
    std::println("{:>10} {:>14} {:>14} {:>10} {:>14} {:>26}", "particles", "CPU ms/step", "GPU ms/step", "speedup", "GPU steps/s", "frames/s at 8 steps/frame");

    for (const int n : { 10, 15, 20, 30, 40, 50, 60, 70, 80 })
    {

        const Scenario scenario { make_scenario(n) };

        GpuSimulation gpu { graphics, scenario.points, scenario.bounds_min, scenario.bounds_max };
        const double gpu_ms { time_gpu(graphics, gpu, 30, 100) };

        // the CPU version is only timed while it is still bearable to wait for
        std::string cpu_text { "-" };
        std::string speedup_text { "-" };

        if (n <= 40)
        {

            Simulation cpu { scenario.points, scenario.bounds_min, scenario.bounds_max };
            const double cpu_ms { time_cpu(cpu, 3, n <= 20 ? 20 : 5) };
            cpu_text = std::format("{:.2f}", cpu_ms);
            speedup_text = std::format("{:.1f}x", cpu_ms / gpu_ms);

        }

        // did anything blow up? every particle should still be a real number inside the tank
        const std::vector<::DirectX::XMFLOAT4> final_positions { gpu.read_positions(graphics) };
        std::size_t bad { 0uz };

        for (const auto& p : final_positions)
        {

            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) || p.x < scenario.bounds_min.x - 0.01f || p.x > scenario.bounds_max.x + 0.01f || p.y < scenario.bounds_min.y - 0.01f || p.y > scenario.bounds_max.y + 0.01f || p.z < scenario.bounds_min.z - 0.01f || p.z > scenario.bounds_max.z + 0.01f)
            {

                ++bad;

            }

        }

        std::println("{:>10} {:>14} {:>14.3f} {:>10} {:>14.0f} {:>26.0f}{}", scenario.points.size(), cpu_text, gpu_ms, speedup_text, 1000.0 / gpu_ms, 1000.0 / (8.0 * gpu_ms), bad > 0uz ? std::format("   ({} particles escaped or became nan)", bad) : std::string {});

    }

}
