#ifndef SIMULATION_HH
#define SIMULATION_HH

#include <DirectXMath.h>

#include <vector>

class Simulation
{
public:

    // takes in a vector of starting positions and sets every velocity to zero.
    explicit Simulation(std::vector<::DirectX::XMFLOAT4> positions);
    ~Simulation() = default;
    
    Simulation(const Simulation&) = default;
    Simulation& operator=(const Simulation&) = default;
    
    Simulation(Simulation&&) = default;
    Simulation& operator=(Simulation&&) = default;

    // advances every particle by delta_time seconds: applies gravity, moves them, and bounces them off the tank walls
    void step(float delta_time);

    // current particle positions (w = 1), laid out so they can be uploaded straight to the GPU buffer
    const std::vector<::DirectX::XMFLOAT4>& positions() const;

private:
    
    static constexpr float gravity { -9.8f }; // gravity is 9.8 m/s

    std::vector<::DirectX::XMFLOAT4> m_positions {};  // positions of each particle; m_positions[i] = position vector of the ith particle
    std::vector<::DirectX::XMFLOAT3> m_velocities {}; // velocities of each particle, m_velocities[i] = velocity vector of the ith particle

    // define boundaries of a box for bouncing
    DirectX::XMFLOAT3 m_bounds_min { -1.5f, -1.5f, -1.5f };
    DirectX::XMFLOAT3 m_bounds_max {  1.5f,  1.5f,  1.5f };

    float m_damping { 0.5f }; // how much energy a particle loses when bouncing

};



#endif // SIMULATION_HH