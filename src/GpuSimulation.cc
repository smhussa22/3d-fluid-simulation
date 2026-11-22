#include "GpuSimulation.hh"

#include <algorithm>
#include <cmath>
#include <utility>

GpuSimulation::GpuSimulation(const Graphics& graphics, std::vector<::DirectX::XMFLOAT4> positions) : m_particle_count { positions.size() }, m_initial_positions { std::move(positions) }
{

    // size the neighbor search grid to fit the tank
    m_cells_x = std::max(1, static_cast<int>(std::ceil((m_bounds_max.x - m_bounds_min.x) / cell_size)));
    m_cells_y = std::max(1, static_cast<int>(std::ceil((m_bounds_max.y - m_bounds_min.y) / cell_size)));
    m_cells_z = std::max(1, static_cast<int>(std::ceil((m_bounds_max.z - m_bounds_min.z) / cell_size)));
    m_cell_count = static_cast<std::size_t>(m_cells_x) * static_cast<std::size_t>(m_cells_y) * static_cast<std::size_t>(m_cells_z);

    // per-particle buffers: positions start from the given values, everything else starts zeroed
    m_positions = std::make_unique<Buffer>(graphics, sizeof(::DirectX::XMFLOAT4), m_particle_count, m_initial_positions.data());
    m_velocities = std::make_unique<Buffer>(graphics, sizeof(::DirectX::XMFLOAT4), m_particle_count, nullptr);
    m_densities = std::make_unique<Buffer>(graphics, sizeof(float), m_particle_count, nullptr);
    m_pressures = std::make_unique<Buffer>(graphics, sizeof(float), m_particle_count, nullptr);
    m_accelerations = std::make_unique<Buffer>(graphics, sizeof(::DirectX::XMFLOAT4), m_particle_count, nullptr);
    m_particle_cell = std::make_unique<Buffer>(graphics, sizeof(std::uint32_t), m_particle_count, nullptr);
    m_sorted_indices = std::make_unique<Buffer>(graphics, sizeof(std::uint32_t), m_particle_count, nullptr);

    // per-cell buffers (cell_start has one extra entry that marks the end)
    m_cell_counts = std::make_unique<Buffer>(graphics, sizeof(std::uint32_t), m_cell_count, nullptr);
    m_cell_start = std::make_unique<Buffer>(graphics, sizeof(std::uint32_t), m_cell_count + 1uz, nullptr);
    m_cell_fill = std::make_unique<Buffer>(graphics, sizeof(std::uint32_t), m_cell_count, nullptr);

    // the shared constants
    m_params = std::make_unique<ConstantBuffer>(graphics, sizeof(Params));

    // the compute passes written so far
    m_integrate_shader = std::make_unique<Shader>(graphics, L"shaders/integrate.hlsl");

}

void GpuSimulation::step(const Graphics& graphics, float delta_time)
{

    _update_params(graphics, delta_time);

    // every pass reads the same shared constants, in slot b0
    m_params->bind_compute(graphics, 0u);

    // how many groups of 256 threads it takes to give every particle one thread (rounded up)
    const ::UINT groups { static_cast<::UINT>((m_particle_count + 255uz) / 256uz) };

    // integrate: apply gravity, move the particles, bounce them off the walls (writes positions and velocities)
    ::ID3D11UnorderedAccessView* integrate_views[2] { m_positions->unordered_access_view(), m_velocities->unordered_access_view() };
    graphics.context()->CSSetUnorderedAccessViews(0u, 2u, integrate_views, nullptr);
    m_integrate_shader->use(graphics);
    m_integrate_shader->dispatch(graphics, groups, 1u, 1u);

    // unbind, so the renderer can read the positions buffer without a read/write conflict
    ::ID3D11UnorderedAccessView* none[2] { nullptr, nullptr };
    graphics.context()->CSSetUnorderedAccessViews(0u, 2u, none, nullptr);

    // more passes get added above this line, in the order listed in the header, as each shader is written

}

void GpuSimulation::reset(const Graphics& graphics)
{

    // put the particles back where they started and stop them moving
    const std::vector<::DirectX::XMFLOAT4> still(m_particle_count, ::DirectX::XMFLOAT4 { 0.0f, 0.0f, 0.0f, 0.0f });
    m_positions->write(graphics, m_initial_positions.data());
    m_velocities->write(graphics, still.data());

}

const ::DirectX::XMFLOAT3& GpuSimulation::boundsMin() const
{

    return m_bounds_min;

}

const ::DirectX::XMFLOAT3& GpuSimulation::boundsMax() const
{

    return m_bounds_max;

}

GpuSimulation::Settings& GpuSimulation::settings()
{

    return m_settings;

}

const Buffer& GpuSimulation::position_buffer() const
{

    return *m_positions;

}

std::size_t GpuSimulation::particle_count() const
{

    return m_particle_count;

}

void GpuSimulation::_update_params(const Graphics& graphics, float delta_time)
{

    const Params params
    {
        static_cast<std::uint32_t>(m_particle_count), delta_time, kernel_radius, particle_mass,
        rest_density, m_settings.stiffness, m_settings.viscosity, m_settings.damping,
        m_settings.gravity, cell_size, static_cast<std::uint32_t>(m_cells_x), static_cast<std::uint32_t>(m_cells_y),
        static_cast<std::uint32_t>(m_cells_z), static_cast<std::uint32_t>(m_cell_count), 0.0f, 0.0f,
        { m_bounds_min.x, m_bounds_min.y, m_bounds_min.z, 0.0f },
        { m_bounds_max.x, m_bounds_max.y, m_bounds_max.z, 0.0f }
    };

    m_params->write(graphics, &params);

}
