#include "GpuSimulation.hh"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "check.hh"

GpuSimulation::GpuSimulation(const Graphics& graphics, std::vector<::DirectX::XMFLOAT4> positions, const ::DirectX::XMFLOAT3& bounds_min, const ::DirectX::XMFLOAT3& bounds_max, float particle_spacing) : m_particle_count { positions.size() }, m_initial_positions { std::move(positions) }, m_bounds_min { bounds_min }, m_bounds_max { bounds_max }
{

    // everything that depends on how big the particles are
    m_kernel_radius = 2.0f * particle_spacing;
    m_cell_size = m_kernel_radius;
    m_particle_mass = rest_density * particle_spacing * particle_spacing * particle_spacing;

    // size the neighbor search grid to fit the tank
    m_cells_x = std::max(1, static_cast<int>(std::ceil((m_bounds_max.x - m_bounds_min.x) / m_cell_size)));
    m_cells_y = std::max(1, static_cast<int>(std::ceil((m_bounds_max.y - m_bounds_min.y) / m_cell_size)));
    m_cells_z = std::max(1, static_cast<int>(std::ceil((m_bounds_max.z - m_bounds_min.z) / m_cell_size)));
    m_cell_count = static_cast<std::size_t>(m_cells_x) * static_cast<std::size_t>(m_cells_y) * static_cast<std::size_t>(m_cells_z);

    // per-particle buffers: positions start from the given values, everything else starts zeroed
    m_positions = std::make_unique<Buffer>(graphics, sizeof(::DirectX::XMFLOAT4), m_particle_count, m_initial_positions.data());
    m_velocities = std::make_unique<Buffer>(graphics, sizeof(::DirectX::XMFLOAT4), m_particle_count, nullptr);
    m_positions_sorted = std::make_unique<Buffer>(graphics, sizeof(::DirectX::XMFLOAT4), m_particle_count, nullptr);
    m_velocities_sorted = std::make_unique<Buffer>(graphics, sizeof(::DirectX::XMFLOAT4), m_particle_count, nullptr);
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

    // GPU clock queries for profiling
    ::D3D11_QUERY_DESC timestamp_desc {};
    timestamp_desc.Query = D3D11_QUERY_TIMESTAMP;

    for (auto& timestamp : m_timestamps)
    {

        check(graphics.device()->CreateQuery(&timestamp_desc, &timestamp), "CreateQuery (timestamp)");

    }

    ::D3D11_QUERY_DESC disjoint_desc {};
    disjoint_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    check(graphics.device()->CreateQuery(&disjoint_desc, &m_timestamp_disjoint), "CreateQuery (timestamp disjoint)");

    // one compute shader per pass
    m_clear_cells_shader = std::make_unique<Shader>(graphics, L"shaders/clear_cells.hlsl");
    m_count_cells_shader = std::make_unique<Shader>(graphics, L"shaders/count_cells.hlsl");
    m_prefix_sum_shader = std::make_unique<Shader>(graphics, L"shaders/prefix_sum.hlsl");
    m_scatter_shader = std::make_unique<Shader>(graphics, L"shaders/scatter.hlsl");
    m_reorder_shader = std::make_unique<Shader>(graphics, L"shaders/reorder.hlsl");
    m_density_shader = std::make_unique<Shader>(graphics, L"shaders/density.hlsl");
    m_pressure_shader = std::make_unique<Shader>(graphics, L"shaders/pressure.hlsl");
    m_forces_shader = std::make_unique<Shader>(graphics, L"shaders/forces.hlsl");
    m_integrate_shader = std::make_unique<Shader>(graphics, L"shaders/integrate.hlsl");

}

void GpuSimulation::step(const Graphics& graphics, float delta_time)
{

    _update_params(graphics, delta_time);

    // every pass reads the same shared constants, in slot b0
    m_params->bind_compute(graphics, 0u);

    // how many groups of threads it takes to give every particle (or every cell) one thread, rounded up
    const ::UINT particle_groups { static_cast<::UINT>((m_particle_count + 255uz) / 256uz) };
    const ::UINT cell_groups { static_cast<::UINT>((m_cell_count + 255uz) / 256uz) };

    // when profiling, take a GPU clock reading before the first pass and after every pass
    std::size_t stamp { 0uz };

    if (m_profiling)
    {

        graphics.context()->Begin(m_timestamp_disjoint.Get());
        graphics.context()->End(m_timestamps[0].Get());

    }

    const auto finish_pass = [&]()
    {

        if (m_profiling)
        {

            ++stamp;
            graphics.context()->End(m_timestamps[stamp].Get());

        }

    };

    // sort the particles into grid cells: reset the counters, count particles per cell, turn counts into start positions, then place each particle in its cell's slots
    _run_pass(graphics, *m_clear_cells_shader, {}, { m_cell_counts.get(), m_cell_fill.get() }, cell_groups);
    finish_pass();
    _run_pass(graphics, *m_count_cells_shader, { m_positions.get() }, { m_particle_cell.get(), m_cell_counts.get() }, particle_groups);
    finish_pass();
    _run_pass(graphics, *m_prefix_sum_shader, { m_cell_counts.get() }, { m_cell_start.get() }, 1u);
    finish_pass();
    _run_pass(graphics, *m_scatter_shader, { m_particle_cell.get(), m_cell_start.get() }, { m_cell_fill.get(), m_sorted_indices.get() }, particle_groups);
    finish_pass();

    // physically reorder the particles by cell (copy into the scratch buffers, then swap), so a cell's particles are next to each other in memory instead of scattered
    _run_pass(graphics, *m_reorder_shader, { m_sorted_indices.get(), m_positions.get(), m_velocities.get() }, { m_positions_sorted.get(), m_velocities_sorted.get() }, particle_groups);
    finish_pass();
    std::swap(m_positions, m_positions_sorted);
    std::swap(m_velocities, m_velocities_sorted);

    // how crowded each particle is, and the pressure that causes
    _run_pass(graphics, *m_density_shader, { m_positions.get(), m_cell_start.get() }, { m_densities.get() }, particle_groups);
    finish_pass();
    _run_pass(graphics, *m_pressure_shader, { m_densities.get() }, { m_pressures.get() }, particle_groups);
    finish_pass();

    // the push (pressure) and drag (viscosity) each particle gets from its neighbors
    _run_pass(graphics, *m_forces_shader, { m_positions.get(), m_velocities.get(), m_densities.get(), m_pressures.get(), m_cell_start.get() }, { m_accelerations.get() }, particle_groups);
    finish_pass();

    // apply those accelerations and gravity, move the particles, bounce them off the walls
    _run_pass(graphics, *m_integrate_shader, { m_accelerations.get() }, { m_positions.get(), m_velocities.get() }, particle_groups);
    finish_pass();

    if (m_profiling)
    {

        graphics.context()->End(m_timestamp_disjoint.Get());

    }

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

float GpuSimulation::kernel_radius() const
{

    return m_kernel_radius;

}

std::size_t GpuSimulation::particle_count() const
{

    return m_particle_count;

}

void GpuSimulation::set_profiling(bool profiling)
{

    m_profiling = profiling;

}

std::array<double, GpuSimulation::pass_count> GpuSimulation::read_pass_times(const Graphics& graphics) const
{

    std::array<double, pass_count> times {};

    // wait for the GPU to finish the profiled step, then read the clock frequency and whether the readings are valid
    ::D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint {};

    while (graphics.context()->GetData(m_timestamp_disjoint.Get(), &disjoint, sizeof(disjoint), 0u) == S_FALSE)
    {
    }

    if (disjoint.Disjoint)
    {

        return times;

    }

    // each pass takes the gap between two clock readings, converted from ticks to milliseconds
    for (std::size_t pass { 0uz }; pass < pass_count; ++pass)
    {

        ::UINT64 begin { 0u };
        ::UINT64 end { 0u };

        while (graphics.context()->GetData(m_timestamps[pass].Get(), &begin, sizeof(begin), 0u) == S_FALSE)
        {
        }

        while (graphics.context()->GetData(m_timestamps[pass + 1uz].Get(), &end, sizeof(end), 0u) == S_FALSE)
        {
        }

        times[pass] = static_cast<double>(end - begin) / static_cast<double>(disjoint.Frequency) * 1000.0;

    }

    return times;

}

std::vector<::DirectX::XMFLOAT4> GpuSimulation::read_positions(const Graphics& graphics) const
{

    std::vector<::DirectX::XMFLOAT4> result(m_particle_count);
    m_positions->read(graphics, result.data());
    return result;

}

std::vector<float> GpuSimulation::read_densities(const Graphics& graphics) const
{

    std::vector<float> result(m_particle_count);
    m_densities->read(graphics, result.data());
    return result;

}

void GpuSimulation::_update_params(const Graphics& graphics, float delta_time)
{

    // the kernel constants only depend on the kernel radius, so work them out here once instead of in every thread for every neighbor
    const double radius { static_cast<double>(m_kernel_radius) };
    const float poly6_coefficient { static_cast<float>(315.0 / (64.0 * std::numbers::pi * std::pow(radius, 9.0))) };
    const float spiky_coefficient { static_cast<float>(45.0 / (std::numbers::pi * std::pow(radius, 6.0))) };

    const Params params
    {
        static_cast<std::uint32_t>(m_particle_count), delta_time, m_kernel_radius, m_particle_mass,
        rest_density, m_settings.stiffness, m_settings.viscosity, m_settings.damping,
        m_settings.gravity, m_cell_size, static_cast<std::uint32_t>(m_cells_x), static_cast<std::uint32_t>(m_cells_y),
        static_cast<std::uint32_t>(m_cells_z), static_cast<std::uint32_t>(m_cell_count), poly6_coefficient, spiky_coefficient,
        { m_bounds_min.x, m_bounds_min.y, m_bounds_min.z, 0.0f },
        { m_bounds_max.x, m_bounds_max.y, m_bounds_max.z, 0.0f }
    };

    m_params->write(graphics, &params);

}

void GpuSimulation::_run_pass(const Graphics& graphics, const Shader& shader, std::initializer_list<const Buffer*> inputs, std::initializer_list<const Buffer*> outputs, ::UINT groups) const
{

    // gather the views for each slot (a compute shader can bind up to 8 read-write views)
    ::ID3D11ShaderResourceView* input_views[8] {};
    ::ID3D11UnorderedAccessView* output_views[8] {};
    ::UINT input_count { 0u };
    ::UINT output_count { 0u };

    for (const Buffer* buffer : inputs)
    {

        input_views[input_count] = buffer->shader_resource_view();
        ++input_count;

    }

    for (const Buffer* buffer : outputs)
    {

        output_views[output_count] = buffer->unordered_access_view();
        ++output_count;

    }

    // bind, run, then unbind so the next pass (or the renderer) can use these buffers the other way round without a read/write conflict
    graphics.context()->CSSetShaderResources(0u, input_count, input_views);
    graphics.context()->CSSetUnorderedAccessViews(0u, output_count, output_views, nullptr);
    shader.use(graphics);
    shader.dispatch(graphics, groups, 1u, 1u);

    ::ID3D11ShaderResourceView* no_inputs[8] {};
    ::ID3D11UnorderedAccessView* no_outputs[8] {};
    graphics.context()->CSSetShaderResources(0u, input_count, no_inputs);
    graphics.context()->CSSetUnorderedAccessViews(0u, output_count, no_outputs, nullptr);

}