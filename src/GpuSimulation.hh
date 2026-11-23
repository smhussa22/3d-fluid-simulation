#ifndef GPU_SIMULATION_HH
#define GPU_SIMULATION_HH

#include "Buffer.hh"
#include "ConstantBuffer.hh"
#include "Graphics.hh"
#include "Shader.hh"
#include "Simulation.hh"

#include <DirectXMath.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <vector>

// the SPH fluid simulation running on the GPU: every particle's data lives in GPU buffers and each step is a series of compute shader passes
class GpuSimulation
{
public:

    // the same live-tunable values the CPU simulation uses, so one panel can drive either
    using Settings = Simulation::Settings;

    // creates every GPU buffer and uploads the starting positions; velocities start at zero
    // particle_spacing is how far apart the particles start; the smoothing radius, grid cell size and particle mass are all worked out from it, so smaller spacing means finer water
    explicit GpuSimulation(const Graphics& graphics, std::vector<::DirectX::XMFLOAT4> positions, const ::DirectX::XMFLOAT3& bounds_min = ::DirectX::XMFLOAT3 { -0.6f, -0.6f, -0.6f }, const ::DirectX::XMFLOAT3& bounds_max = ::DirectX::XMFLOAT3 { 0.6f, 0.6f, 0.6f }, float particle_spacing = 0.1f);
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

    // the GPU buffer of particle densities (float, same order as the positions); the renderer reads this to tell dense water from sparse spray
    const Buffer& density_buffer() const;

    // the GPU buffer of particle velocities (float4, w unused, same order as the positions); the renderer reads this to find how fast each particle moves (for foam)
    const Buffer& velocity_buffer() const;

    // how many particles there are
    std::size_t particle_count() const;

    // the SPH smoothing radius, which is twice the particle spacing
    float kernel_radius() const;

    // how many compute passes make up one step
    static constexpr std::size_t pass_count { 9uz };

    // the name of each pass, in the order they run
    static constexpr std::array<const char*, pass_count> pass_names { "clear", "count", "scan", "scatter", "reorder", "density", "pressure", "forces", "integrate" };

    // when on, step() records GPU timestamps around every pass (slightly slower, for profiling only)
    void set_profiling(bool profiling);

    // waits for the last profiled step and returns how long each pass took on the GPU, in milliseconds
    std::array<double, pass_count> read_pass_times(const Graphics& graphics) const;

    // copies the particle positions back to the CPU (slow; for checking results and debugging only)
    std::vector<::DirectX::XMFLOAT4> read_positions(const Graphics& graphics) const;

    // copies the particle densities back to the CPU (slow; for checking results and debugging only)
    std::vector<float> read_densities(const Graphics& graphics) const;

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
        float poly6_coefficient;      // 315 / (64 pi h^9), worked out once here so the shaders do not repeat it for every neighbor
        float spiky_coefficient;      // 45 / (pi h^6), shared by the pressure and viscosity kernels

        ::DirectX::XMFLOAT4 bounds_min; // tank minimum corner (w unused)
        ::DirectX::XMFLOAT4 bounds_max; // tank maximum corner (w unused)

    };

    static_assert(sizeof(Params) % 16 == 0, "constant buffers must be a multiple of 16 bytes");

    // these three follow from the particle spacing (see the constructor): the smoothing radius is 2 x spacing (about 33 neighbors per particle),
    // the grid cell is as wide as the smoothing radius so every neighbor is in the particle's cell or one of the 26 around it,
    // and the mass is the rest density times the volume each particle fills, so the fluid starts at its rest density
    float m_kernel_radius { 0.2f };  // maximum distance at which two particles influence each other (SPH smoothing radius)
    float m_cell_size { 0.2f };      // width of one grid cell
    float m_particle_mass { 1.0f };  // mass of one particle
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
    std::unique_ptr<Buffer> m_positions_sorted {};  // scratch copy of positions, filled in cell order each step then swapped with m_positions
    std::unique_ptr<Buffer> m_velocities_sorted {}; // scratch copy of velocities, filled in cell order each step then swapped with m_velocities
    std::unique_ptr<Buffer> m_densities {};      // float density of each particle
    std::unique_ptr<Buffer> m_pressures {};      // float pressure of each particle
    std::unique_ptr<Buffer> m_accelerations {};  // float4 acceleration of each particle from pressure and viscosity
    std::unique_ptr<Buffer> m_particle_cell {};  // uint grid cell each particle is in
    std::unique_ptr<Buffer> m_sorted_indices {}; // uint particle indices grouped by grid cell

    // per-cell data (one element per grid cell, except cell_start which has one extra)
    std::unique_ptr<Buffer> m_cell_counts {}; // uint how many particles are in each cell (filled with atomic adds)
    std::unique_ptr<Buffer> m_cell_start {};  // uint where each cell's particles begin in m_sorted_indices (one extra entry marks the end)
    std::unique_ptr<Buffer> m_cell_fill {};   // uint next free slot in each cell while particles are being sorted (atomic adds)

    // GPU timing (only used while profiling)
    bool m_profiling { false }; // whether step() records timestamps
    std::array<::Microsoft::WRL::ComPtr<::ID3D11Query>, pass_count + 1uz> m_timestamps {}; // a GPU clock reading before the first pass and after each pass
    ::Microsoft::WRL::ComPtr<::ID3D11Query> m_timestamp_disjoint {}; // tells us the clock frequency and whether the readings can be trusted

    // the values shared by every pass
    std::unique_ptr<ConstantBuffer> m_params {}; // holds a Params, rewritten at the start of each step

    // one compute shader per pass, created as each pass is written; they run in this order every step
    std::unique_ptr<Shader> m_clear_cells_shader {}; // zeroes the per-cell counters
    std::unique_ptr<Shader> m_count_cells_shader {}; // finds each particle's cell and counts particles per cell
    std::unique_ptr<Shader> m_prefix_sum_shader {};  // turns the counts into cell start positions
    std::unique_ptr<Shader> m_scatter_shader {};     // writes particle indices into their cell's slots
    std::unique_ptr<Shader> m_reorder_shader {};     // copies the particles into cell order so neighbors sit next to each other in memory
    std::unique_ptr<Shader> m_density_shader {};     // computes every particle's density
    std::unique_ptr<Shader> m_pressure_shader {};    // computes every particle's pressure from its density
    std::unique_ptr<Shader> m_forces_shader {};      // computes pressure and viscosity accelerations
    std::unique_ptr<Shader> m_integrate_shader {};   // applies gravity, moves the particles, bounces them off the walls

    // fills in a Params from the current settings and uploads it to the GPU
    void _update_params(const Graphics& graphics, float delta_time);

    // binds the input buffers (read-only, slots t0, t1, ...) and output buffers (read-write, slots u0, u1, ...), runs the shader over the given number of groups, then unbinds everything again
    void _run_pass(const Graphics& graphics, const Shader& shader, std::initializer_list<const Buffer*> inputs, std::initializer_list<const Buffer*> outputs, ::UINT groups) const;

};

#endif // GPU_SIMULATION_HH
