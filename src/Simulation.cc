#include "Simulation.hh"

#include <utility>

Simulation::Simulation(std::vector<::DirectX::XMFLOAT4> positions) : m_positions { std::move(positions) }
{

    m_velocities.resize(m_positions.size());

}

void Simulation::step(float delta_time)
{

    for (auto i {0uz}; i < m_positions.size(); ++i)
    {

        // semi-implicit Euler integration
        auto& position { m_positions[i] };
        auto& velocity { m_velocities[i] };
        velocity.y *= gravity;
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

// current particle positions (w = 1), laid out so they can be uploaded straight to the GPU buffer
const std::vector<::DirectX::XMFLOAT4>& Simulation::positions() const
{
    return m_positions;
}