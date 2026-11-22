#include "Simulation.hh"

#include <algorithm>
#include <utility>
#include <cmath>
#include <numbers>
#include <print>

namespace
{

    // returns how much a neighboring particle contributes to another particle's density based on their squared distance and the kernel radius of the current particle
    // pg 4: https://cgl.ethz.ch/Downloads/Publications/Papers/2003/Mue03b/Mue03b.pdf
    float _poly6 (float distance_squared, float kernel_radius)
    {

        if (distance_squared >= kernel_radius * kernel_radius)
        {
            return 0.0f;
        }

        const float diff { kernel_radius * kernel_radius - distance_squared };
        const float coeff { 315.0f / (64.0f * std::numbers::pi_v<float> * std::pow(kernel_radius, 9.0f)) };
        return coeff * diff * diff * diff;

    }

    // returns the magnitude of the spiky kernel gradient based on distance and influence radius
    // the spikey kernel gradient tells us how strongly a neighboring particle contributes to the pressure force
    float _spiky_gradient(float distance, float kernel_radius)
    {

        if (distance >= kernel_radius)
        {
            return 0.0f;
        }

        const float diff { kernel_radius - distance };
        return (45.0f / (std::numbers::pi_v<float> * std::pow(kernel_radius, 6.0f))) * diff * diff;

    }

    // returns the Laplacian of the viscosity kernel for a given distance and influence radius
    // it tells us how strongly a neighboring particle's velocity difference drags on a particle
    // pg 5: https://cgl.ethz.ch/Downloads/Publications/Papers/2003/Mue03b/Mue03b.pdf
    float _viscosity_laplacian(float distance, float kernel_radius)
    {

        if (distance >= kernel_radius)
        {

            return 0.0f;

        }

        return (45.0f / (std::numbers::pi_v<float> * std::pow(kernel_radius, 6.0f))) * (kernel_radius - distance);

    }

}

Simulation::Simulation(std::vector<::DirectX::XMFLOAT4> positions, const ::DirectX::XMFLOAT3& bounds_min, const ::DirectX::XMFLOAT3& bounds_max) : m_positions { std::move(positions) }, m_bounds_min { bounds_min }, m_bounds_max { bounds_max }
{

    m_initial_positions = m_positions;
    m_velocities.resize(m_positions.size());
    m_densities.resize(m_positions.size()); // we'll calculate densities based on its neighbouring particles
    m_pressures.resize(m_positions.size());
    m_accelerations.resize(m_positions.size());

    // size the neighbor search grid to fit the tank
    m_cells_x = std::max(1, static_cast<int>(std::ceil((m_bounds_max.x - m_bounds_min.x) / cell_size)));
    m_cells_y = std::max(1, static_cast<int>(std::ceil((m_bounds_max.y - m_bounds_min.y) / cell_size)));
    m_cells_z = std::max(1, static_cast<int>(std::ceil((m_bounds_max.z - m_bounds_min.z) / cell_size)));
    m_cell_count = static_cast<std::size_t>(m_cells_x) * static_cast<std::size_t>(m_cells_y) * static_cast<std::size_t>(m_cells_z);
    m_particle_cell.resize(m_positions.size());
    m_cell_start.resize(m_cell_count + 1uz);
    m_sorted_indices.resize(m_positions.size());

    _build_grid();
    _compute_densities();
    _compute_pressures();


}

void Simulation::step(float delta_time)
{

    // the particles moved last step, so re-sort them into grid cells before searching for neighbors
    _build_grid();

    // work out how crowded each particle is, the pressure that causes, and the pushes and drags each particle gets from its neighbors
    _compute_densities();
    _compute_pressures();
    _compute_pressure_forces();
    _compute_viscosity_forces();

    // the physics runs 480 steps a second, so print the middle particle and the most crowded one about once a second
    if (++m_step_count % 480uz == 0uz)
    {

        std::println("density[555] = {:.1f}, max density = {:.1f}", m_densities[555], *std::ranges::max_element(m_densities));

    }

    #pragma omp parallel for
    for (auto i {0uz}; i < m_positions.size(); ++i)
    {

        // semi-implicit Euler integration
        auto& position { m_positions[i] };
        auto& velocity { m_velocities[i] };
        velocity.x += m_accelerations[i].x * delta_time;
        velocity.y += (m_settings.gravity + m_accelerations[i].y) * delta_time; // gravity is acceleration not a multiplier for velocity
        velocity.z += m_accelerations[i].z * delta_time;
        position.x += velocity.x * delta_time;
        position.y += velocity.y * delta_time;
        position.z += velocity.z * delta_time;

        // maybe clean this up later but not a priority
        // collision checks
        if (position.x < m_bounds_min.x)
        {
            position.x = m_bounds_min.x;
            velocity.x *= -m_settings.damping;
        }

        if (position.x > m_bounds_max.x)
        {
            position.x = m_bounds_max.x;
            velocity.x *= -m_settings.damping;
        }

        if (position.y < m_bounds_min.y)
        {
            position.y = m_bounds_min.y;
            velocity.y *= -m_settings.damping;
        }

        if (position.y > m_bounds_max.y)
        {
            position.y = m_bounds_max.y;
            velocity.y *= -m_settings.damping;
        }

        // Z boundaries.
        if (position.z < m_bounds_min.z)
        {
            position.z = m_bounds_min.z;
            velocity.z *= -m_settings.damping;
        }

        if (position.z > m_bounds_max.z)
        {
            position.z = m_bounds_max.z;
            velocity.z *= -m_settings.damping;
        }

    }

}

void Simulation::reset()
{

    m_positions = m_initial_positions;
    std::ranges::fill(m_velocities, ::DirectX::XMFLOAT3 { 0.0f, 0.0f, 0.0f });

}

const std::vector<float>& Simulation::densities() const
{

    return m_densities;

}

const ::DirectX::XMFLOAT3& Simulation::boundsMin() const
{

    return m_bounds_min;

}

const ::DirectX::XMFLOAT3& Simulation::boundsMax() const
{

    return m_bounds_max;

}

Simulation::Settings& Simulation::settings()
{

    return m_settings;

}

void Simulation::setUseGrid(bool use_grid)
{

    m_use_grid = use_grid;

}

// current particle positions (w = 1), laid out so they can be uploaded straight to the GPU buffer
const std::vector<::DirectX::XMFLOAT4>& Simulation::positions() const
{
    return m_positions;
}


// which grid cell (x, y, z) a position falls in
::DirectX::XMINT3 Simulation::_cell_coords(const ::DirectX::XMFLOAT4& position) const
{

    const int x { static_cast<int>(std::floor((position.x - m_bounds_min.x) / cell_size)) };
    const int y { static_cast<int>(std::floor((position.y - m_bounds_min.y) / cell_size)) };
    const int z { static_cast<int>(std::floor((position.z - m_bounds_min.z) / cell_size)) };

    return { std::clamp(x, 0, m_cells_x - 1), std::clamp(y, 0, m_cells_y - 1), std::clamp(z, 0, m_cells_z - 1) };

}

// sorts the particles into grid cells with a counting sort
void Simulation::_build_grid()
{

    // work out which cell each particle is in
    for (auto i { 0uz }; i < m_positions.size(); ++i)
    {

        const ::DirectX::XMINT3 cell { _cell_coords(m_positions[i]) };
        m_particle_cell[i] = static_cast<std::size_t>(cell.x) + static_cast<std::size_t>(cell.y) * static_cast<std::size_t>(m_cells_x) + static_cast<std::size_t>(cell.z) * static_cast<std::size_t>(m_cells_x) * static_cast<std::size_t>(m_cells_y);

    }

    // count how many particles are in each cell (stored one slot ahead so the running total below lines up)
    std::ranges::fill(m_cell_start, 0uz);

    for (auto i { 0uz }; i < m_positions.size(); ++i)
    {

        ++m_cell_start[m_particle_cell[i] + 1uz];

    }

    // running total: m_cell_start[c] now holds how many particles are in all earlier cells, i.e. where cell c begins
    for (auto c { 1uz }; c <= m_cell_count; ++c)
    {

        m_cell_start[c] += m_cell_start[c - 1uz];

    }

    // drop each particle into the next free slot of its cell
    std::vector<std::size_t> next_slot(m_cell_start.begin(), m_cell_start.end() - 1);

    for (auto i { 0uz }; i < m_positions.size(); ++i)
    {

        const std::size_t cell { m_particle_cell[i] };
        m_sorted_indices[next_slot[cell]] = i;
        ++next_slot[cell];

    }

}

// calls function(j) for every particle j that could be near particle i
template <typename Function>
void Simulation::_for_each_neighbor(std::size_t i, Function&& function) const
{

    // without the grid, every particle is a candidate
    if (!m_use_grid)
    {

        for (auto j { 0uz }; j < m_positions.size(); ++j)
        {

            function(j);

        }

        return;

    }

    // with the grid, only the particle's own cell and the 26 around it can hold neighbors
    const ::DirectX::XMINT3 center { _cell_coords(m_positions[i]) };

    for (int dz { -1 }; dz <= 1; ++dz)
    {

        const int z { center.z + dz };

        if (z < 0 || z >= m_cells_z)
        {

            continue;

        }

        for (int dy { -1 }; dy <= 1; ++dy)
        {

            const int y { center.y + dy };

            if (y < 0 || y >= m_cells_y)
            {

                continue;

            }

            for (int dx { -1 }; dx <= 1; ++dx)
            {

                const int x { center.x + dx };

                if (x < 0 || x >= m_cells_x)
                {

                    continue;

                }

                const std::size_t cell { static_cast<std::size_t>(x) + static_cast<std::size_t>(y) * static_cast<std::size_t>(m_cells_x) + static_cast<std::size_t>(z) * static_cast<std::size_t>(m_cells_x) * static_cast<std::size_t>(m_cells_y) };

                for (auto k { m_cell_start[cell] }; k < m_cell_start[cell + 1uz]; ++k)
                {

                    function(m_sorted_indices[k]);

                }

            }

        }

    }

}

// works out the density of every particle from its neighbors' positions
void Simulation::_compute_densities()
{

    // each particle only writes its own results, so the loop can be split across threads
    #pragma omp parallel for
    for (auto i { 0uz }; i < m_positions.size(); ++i)
    {

        float density { 0.0f };

        _for_each_neighbor(i, [&](std::size_t j)
        {

            const float delta_x { m_positions[i].x - m_positions[j].x };
            const float delta_y { m_positions[i].y - m_positions[j].y };
            const float delta_z { m_positions[i].z - m_positions[j].z };
            const float distance_squared { delta_x * delta_x + delta_y * delta_y + delta_z * delta_z };

            density += particle_mass * _poly6(distance_squared, kernel_radius);

        });

        m_densities[i] = density;

    }

}

void Simulation::_compute_pressures()
{

    // each particle only writes its own results, so the loop can be split across threads
    #pragma omp parallel for
    for (auto i { 0uz }; i < m_positions.size(); ++i)
    {

        m_pressures[i] = std::max(0.0f, m_settings.stiffness * (m_densities[i] - rest_density)); // we dont want particles pulling each other together; only pushing

    }

}

void Simulation::_compute_pressure_forces()
{

    // each particle only writes its own results, so the loop can be split across threads
    #pragma omp parallel for
    for (auto i { 0uz }; i < m_positions.size(); ++i)
    {

        ::DirectX::XMFLOAT3 force { 0.0f, 0.0f, 0.0f };

        _for_each_neighbor(i, [&](std::size_t j)
        {

            if (j == i)
            {

                return;

            }

            // get a vector pointing from particle j to particle i (d' = x'_i - x'_j)
            const float delta_x { m_positions[i].x - m_positions[j].x };
            const float delta_y { m_positions[i].y - m_positions[j].y };
            const float delta_z { m_positions[i].z - m_positions[j].z };

            const float distance_squared { delta_x * delta_x + delta_y * delta_y + delta_z * delta_z };

            if (distance_squared >= kernel_radius * kernel_radius || distance_squared == 0.0f)
            {

                return;

            }

            const float distance { std::sqrt(distance_squared) };

            // normalize the vector aka turn it into a unit vector
            const ::DirectX::XMFLOAT3 direction { delta_x / distance, delta_y / distance, delta_z / distance };

            // SPH pressure force equation; add this neighbor's pressure force to force
            // iow calculate how much particle j pushes particle i due to pressure
            const float pressure_term { particle_mass * (m_pressures[i] + m_pressures[j]) / (2.0f * m_densities[j]) * _spiky_gradient(distance, kernel_radius) };

            force.x += pressure_term * direction.x;
            force.y += pressure_term * direction.y;
            force.z += pressure_term * direction.z;

        });

        // force -> acceleration (a = F / density)
        m_accelerations[i] = { force.x / m_densities[i], force.y / m_densities[i], force.z / m_densities[i] };

    }

}

// adds the viscosity acceleration to each particle (pressure forces must already have been computed this step)
void Simulation::_compute_viscosity_forces()
{

    // each particle only writes its own results, so the loop can be split across threads
    #pragma omp parallel for
    for (auto i { 0uz }; i < m_positions.size(); ++i)
    {

        ::DirectX::XMFLOAT3 force { 0.0f, 0.0f, 0.0f };

        _for_each_neighbor(i, [&](std::size_t j)
        {

            if (j == i)
            {

                return;

            }

            const float delta_x { m_positions[i].x - m_positions[j].x };
            const float delta_y { m_positions[i].y - m_positions[j].y };
            const float delta_z { m_positions[i].z - m_positions[j].z };
            const float distance { std::sqrt(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z) };

            if (distance >= kernel_radius)
            {

                return;

            }

            const float kernel { _viscosity_laplacian(distance, kernel_radius) };

            // your turn: add this neighbor's viscosity force to `force`, using viscosity, particle_mass,
            // the velocity DIFFERENCE between j and i (m_velocities[j] - m_velocities[i]), m_densities[j] and `kernel`
            const float velocity_coefficient { m_settings.viscosity * particle_mass / m_densities[j] * kernel };

            const float velocity_diff_x { m_velocities[j].x - m_velocities[i].x };
            const float velocity_diff_y { m_velocities[j].y - m_velocities[i].y };
            const float velocity_diff_z { m_velocities[j].z - m_velocities[i].z };

            force.x += velocity_coefficient * velocity_diff_x;
            force.y += velocity_coefficient * velocity_diff_y;
            force.z += velocity_coefficient * velocity_diff_z;

        });

        // force -> acceleration (a = F / density), added on top of the pressure acceleration
        m_accelerations[i].x += force.x / m_densities[i];
        m_accelerations[i].y += force.y / m_densities[i];
        m_accelerations[i].z += force.z / m_densities[i];

    }

}
