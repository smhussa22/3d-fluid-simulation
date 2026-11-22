#include "Benchmark.hh"

#include "GpuSimulation.hh"
#include "Scenario.hh"
#include "Simulation.hh"

#include <DirectXMath.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <omp.h>
#include <format>
#include <print>
#include <string>
#include <vector>

namespace
{

    // the largest difference between two lists of numbers after sorting both (so the order the particles are stored in does not matter)
    float max_sorted_difference(std::vector<float> a, std::vector<float> b)
    {

        std::ranges::sort(a);
        std::ranges::sort(b);
        float largest { 0.0f };

        for (std::size_t i { 0uz }; i < a.size(); ++i)
        {

            largest = std::max(largest, std::abs(a[i] - b[i]));

        }

        return largest;

    }
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
    // (the GPU keeps its particles in grid-cell order, so particle i on the GPU is not particle i on the CPU; instead each list is sorted first, which is valid
    // because sorting two lists never makes their largest element-by-element difference bigger)
    {

        const Scenario scenario { make_scenario(10) };
        Simulation cpu { scenario.points, scenario.bounds_min, scenario.bounds_max };
        GpuSimulation gpu { graphics, scenario.points, scenario.bounds_min, scenario.bounds_max };

        // after one step, both have computed densities from the same positions
        cpu.step(step_time);
        gpu.step(graphics, step_time);
        const float max_density_difference { max_sorted_difference(gpu.read_densities(graphics), cpu.densities()) };

        // after 100 more steps, compare where the particles ended up, one coordinate at a time
        for (int s { 0 }; s < 100; ++s)
        {

            cpu.step(step_time);
            gpu.step(graphics, step_time);

        }

        const std::vector<::DirectX::XMFLOAT4> gpu_positions { gpu.read_positions(graphics) };
        float max_position_difference { 0.0f };

        for (const auto coordinate : { &::DirectX::XMFLOAT4::x, &::DirectX::XMFLOAT4::y, &::DirectX::XMFLOAT4::z })
        {

            std::vector<float> from_gpu {};
            std::vector<float> from_cpu {};

            for (std::size_t i { 0uz }; i < gpu_positions.size(); ++i)
            {

                from_gpu.push_back(gpu_positions[i].*coordinate);
                from_cpu.push_back(cpu.positions()[i].*coordinate);

            }

            max_position_difference = std::max(max_position_difference, max_sorted_difference(from_gpu, from_cpu));

        }

        std::println("validation, {} particles: max density difference after 1 step = {:.2e} kg/m^3, max position difference after 101 steps = {:.2e} m", gpu_positions.size(), max_density_difference, max_position_difference);

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

void run_profile(const Graphics& graphics, int particles)
{

    // the same drop as the interactive window: a cube starting 0.6 m above the floor
    const int side { std::max(1, static_cast<int>(std::lround(std::cbrt(static_cast<double>(particles))))) };
    const Scenario scenario { make_scenario(side, 0.6f) };
    GpuSimulation gpu { graphics, scenario.points, scenario.bounds_min, scenario.bounds_max };
    std::println("{} particles, physics step = 1/480 s. milliseconds per pass for one step, sampled every 0.125 s of simulated time:", scenario.points.size());

    std::string header { std::format("{:>6} {:>8}", "t (s)", "total") };

    for (const char* name : GpuSimulation::pass_names)
    {

        header += std::format(" {:>8}", name);

    }

    std::println("{} {:>12}", header, "max density");

    for (int step { 0 }; step < 4800; ++step)
    {

        // time one step in every 60 (profiling adds a little overhead, so the rest run normally)
        const bool sample { step % 60 == 0 };
        gpu.set_profiling(sample);
        gpu.step(graphics, step_time);

        if (!sample)
        {

            continue;

        }

        const auto times { gpu.read_pass_times(graphics) };
        const std::vector<float> densities { gpu.read_densities(graphics) };
        double total { 0.0 };
        std::string row {};

        for (const double t : times)
        {

            total += t;
            row += std::format(" {:>8.3f}", t);

        }

        std::println("{:>6.3f} {:>8.3f}{} {:>12.0f}", static_cast<double>(step) * static_cast<double>(step_time), total, row, static_cast<double>(*std::max_element(densities.begin(), densities.end())));

    }

}