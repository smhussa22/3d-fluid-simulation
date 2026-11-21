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
    
}

Simulation::Simulation(std::vector<::DirectX::XMFLOAT4> positions) : m_positions { std::move(positions) }
{

    m_initial_positions = m_positions;
    m_velocities.resize(m_positions.size());
    m_densities.resize(m_positions.size()); // we'll calculate densities based on its neighbouring particles
    _compute_densities();

    //
    std::println("{}", m_densities[0]);
    std::println("{}", m_densities[555]);
    
}

void Simulation::step(float delta_time)
{

    // work out how crowded each particle is at its current position
    _compute_densities();

    // the physics runs 120 steps a second, so print the middle particle and the most crowded one about once a second
    if (++m_step_count % 120uz == 0uz)
    {

        std::println("density[555] = {:.1f}, max density = {:.1f}", m_densities[555], *std::ranges::max_element(m_densities));

    }

    for (auto i {0uz}; i < m_positions.size(); ++i)
    {

        // semi-implicit Euler integration
        auto& position { m_positions[i] };
        auto& velocity { m_velocities[i] };
        velocity.y += gravity * delta_time; // gravity is acceleration not a multiplier for velocity
        position.x += velocity.x * delta_time;
        position.y += velocity.y * delta_time;
        position.z += velocity.z * delta_time;

        // maybe clean this up later but not a priority
        // collision checks
        if (position.x < m_bounds_min.x)
        {
            position.x = m_bounds_min.x;
            velocity.x *= -m_damping;
        }

        if (position.x > m_bounds_max.x)
        {
            position.x = m_bounds_max.x;
            velocity.x *= -m_damping;
        }

        if (position.y < m_bounds_min.y)
        {
            position.y = m_bounds_min.y;
            velocity.y *= -m_damping;
        }

        if (position.y > m_bounds_max.y)
        {
            position.y = m_bounds_max.y;
            velocity.y *= -m_damping;
        }

        // Z boundaries.
        if (position.z < m_bounds_min.z)
        {
            position.z = m_bounds_min.z;
            velocity.z *= -m_damping;
        }

        if (position.z > m_bounds_max.z)
        {
            position.z = m_bounds_max.z;
            velocity.z *= -m_damping;
        }

    }

}

void Simulation::reset()
{

    m_positions = m_initial_positions;
    std::ranges::fill(m_velocities, ::DirectX::XMFLOAT3 { 0.0f, 0.0f, 0.0f });

}

// current particle positions (w = 1), laid out so they can be uploaded straight to the GPU buffer
const std::vector<::DirectX::XMFLOAT4>& Simulation::positions() const
{
    return m_positions;
}

// works out the density of every particle from its neighbors' positions
void Simulation::_compute_densities()
{
    for (auto i {0uz}; i < m_positions.size(); ++i)
    {
        m_densities[i] = 0.0f;
        for (auto j {0uz}; j < m_positions.size(); ++j)
        {

            float delta_x { m_positions[i].x - m_positions[j].x };
            float delta_y { m_positions[i].y - m_positions[j].y };
            float delta_z { m_positions[i].z - m_positions[j].z };
            float distance_squared { delta_x * delta_x + delta_y * delta_y + delta_z * delta_z };

            m_densities[i] += particle_mass * _poly6(distance_squared, kernel_radius); 

        }
    }

}