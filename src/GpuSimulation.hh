#ifndef GPU_SIMULATION_HH
#define GPU_SIMULATION_HH

#include "Buffer.hh"
#include "ConstantBuffer.hh"
#include "Graphics.hh"
#include "Shader.hh"
#include "Simulation.hh"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

// the SPH fluid simulation running on the GPU: every particle's data lives in GPU buffers and each step is a series of compute shader passes
class GpuSimulation
{
public:

    // the same live-tunable values the CPU simulation uses, so one panel can drive either
    using Settings = Simulation::Settings;

    // creates every GPU buffer and uploads the starting positions; velocities start at zero
    explicit GpuSimulation(const Graphics& graphics, std::vector<::DirectX::XMFLOAT4> positions);
    ~GpuSimulation() = default;

    // the GPU buffers can't be copied
    GpuSimulation(const GpuSimulation&) = delete;
    GpuSimulation& operator=(const GpuSimulation&) = delete;

    GpuSimulation(GpuSimulation&&) = default;
    GpuSimulation& operator=(GpuSimulation&&) = default;

    // advances every particle by delta_time seconds by running the compute passes on the GPU
    void step(const Graphics& graphics, float delta_time);

    // puts every particle back at its starting position and stops it moving
    void reset(const Graphics& graphics);

    // the corners of the tank, so the renderer can draw it
    const ::DirectX::XMFLOAT3& boundsMin() const;
    const ::DirectX::XMFLOAT3& boundsMax() const;

    // the live-tunable settings, so a UI can read and change them
    Settings& settings();

    // the GPU buffer of particle positions (float4, w = 1); the renderer reads this directly
    const Buffer& position_buffer() const;

    // how many particles there are
    std::size_t particle_count() const;

private:

    // everything the compute shaders need that isn't per-particle; the layout must match the cbuffer in the hlsl exactly
    // (every row below is 16 bytes, which is how the GPU packs constants)
    struct Params
    {

        std::uint32_t particle_count; // number of particles
        float delta_time;             // seconds this step advances
        float kernel_radius;          // SPH smoothing radius
        float particle_mass;          // mass of one particle

        float rest_density;           // density the fluid wants to be at
        float stiffness;              // from Settings
        float viscosity;              // from Settings
        float damping;                // from Settings

        float gravity;                // from Settings
        float cell_size;              // width of one grid cell
        std::uint32_t cells_x;        // grid cells along x
        std::uint32_t cells_y;        // grid cells along y

        std::uint32_t cells_z;        // grid cells along z
        std::uint32_t cell_count;     // total grid cells
        float padding_a;              // unused, rounds the row up to 16 bytes
        float padding_b;              // unused, rounds the row up to 16 bytes

        ::DirectX::XMFLOAT4 bounds_min; // tank minimum corner (w unused)
        ::DirectX::XMFLOAT4 bounds_max; // tank maximum corner (w unused)

    };

    static_assert(sizeof(Params) % 16 == 0, "constant buffers must be a multiple of 16 bytes");

    static constexpr float kernel_radius { 0.2f };   // maximum distance at which two particles influence each other (SPH smoothing radius)
    static constexpr float cell_size { kernel_radius }; // width of one grid cell; equal to the kernel radius so every neighbor is in the particle's cell or one of the 26 around it

    // each particle's mass; 1.0 kg because particles are spaced 0.1m apart in each dimension, representing 0.001m^3 of water at 1000 kg/m^3
    static constexpr float particle_mass { 1.0f };

    static constexpr float rest_density { 1000.0f }; // water has a density of 1000 kg per cubed metre

    std::size_t m_particle_count { 0uz }; // how many particles there are
    std::vector<::DirectX::XMFLOAT4> m_initial_positions {}; // the starting positions, kept on the CPU so reset() can upload them again

    // define boundaries of a box for bouncing
    ::DirectX::XMFLOAT3 m_bounds_min { -0.6f, -0.6f, -0.6f };
    ::DirectX::XMFLOAT3 m_bounds_max {  0.6f,  0.6f,  0.6f };

    Settings m_settings {}; // the live-tunable values (changed from the controls panel)

    // the neighbor search grid: the tank is cut into cells, and particles are sorted by cell
    int m_cells_x { 1 }; // number of grid cells along x
    int m_cells_y { 1 }; // number of grid cells along y
    int m_cells_z { 1 }; // number of grid cells along z
    std::size_t m_cell_count { 1uz }; // total number of grid cells

    // per-particle data (one element per particle)
    std::unique_ptr<Buffer> m_positions {};      // float4 positions (the renderer also reads this)
    std::unique_ptr<Buffer> m_velocities {};     // float4 velocities (w unused)
    std::unique_ptr<Buffer> m_densities {};      // float density of each particle
    std::unique_ptr<Buffer> m_pressures {};      // float pressure of each particle
    std::unique_ptr<Buffer> m_accelerations {};  // float4 acceleration of each particle from pressure and viscosity
    std::unique_ptr<Buffer> m_particle_cell {};  // uint grid cell each particle is in
    std::unique_ptr<Buffer> m_sorted_indices {}; // uint particle indices grouped by grid cell

    // per-cell data (one element per grid cell, except cell_start which has one extra)
    std::unique_ptr<Buffer> m_cell_counts {}; // uint how many particles are in each cell (filled with atomic adds)
    std::unique_ptr<Buffer> m_cell_start {};  // uint where each cell's particles begin in m_sorted_indices (one extra entry marks the end)
    std::unique_ptr<Buffer> m_cell_fill {};   // uint next free slot in each cell while particles are being sorted (atomic adds)

    // the values shared by every pass
    std::unique_ptr<ConstantBuffer> m_params {}; // holds a Params, rewritten at the start of each step

    // one compute shader per pass, created as each pass is written; they run in this order every step
    std::unique_ptr<Shader> m_clear_cells_shader {}; // zeroes the per-cell counters
    std::unique_ptr<Shader> m_count_cells_shader {}; // finds each particle's cell and counts particles per cell
    std::unique_ptr<Shader> m_prefix_sum_shader {};  // turns the counts into cell start positions
    std::unique_ptr<Shader> m_scatter_shader {};     // writes particle indices into their cell's slots
    std::unique_ptr<Shader> m_density_shader {};     // computes every particle's density
    std::unique_ptr<Shader> m_pressure_shader {};    // computes every particle's pressure from its density
    std::unique_ptr<Shader> m_forces_shader {};      // computes pressure and viscosity accelerations
    std::unique_ptr<Shader> m_integrate_shader {};   // applies gravity, moves the particles, bounces them off the walls

    // fills in a Params from the current settings and uploads it to the GPU
    void _update_params(const Graphics& graphics, float delta_time);

};

#endif // GPU_SIMULATION_HH
