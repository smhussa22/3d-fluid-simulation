#ifndef SIMULATION_HH
#define SIMULATION_HH

#include <DirectXMath.h>

#include <vector>

class Simulation
{
public:

    // takes in a vector of starting positions and sets every velocity & to zero.
    explicit Simulation(std::vector<::DirectX::XMFLOAT4> positions);
    ~Simulation() = default;
    
    Simulation(const Simulation&) = default;
    Simulation& operator=(const Simulation&) = default;
    
    Simulation(Simulation&&) = default;
    Simulation& operator=(Simulation&&) = default;

    // advances every particle by delta_time seconds: applies gravity, moves them, and bounces them off the tank walls
    void step(float delta_time);

    // puts every particle back at its starting position and stops it moving
    void reset();

    // current particle positions (w = 1), laid out so they can be uploaded straight to the GPU buffer
    const std::vector<::DirectX::XMFLOAT4>& positions() const;

private:
    
    static constexpr float gravity { -9.8f };        // gravity is 9.8 m/s
    static constexpr float kernel_radius { 0.2f };   // maximum distance at which two particles influence each other (SPH smoothing radius)
    
    // each particle's mass; 1.0 kg because particles are spaced 0.1m apart in each dimension, representing 0.001m^3 of water at 1000 kg/m^3
    static constexpr float particle_mass { 1.0f };   
    
    static constexpr float rest_density { 1000.0f }; // water has a density of 1000 kg per cubed metre

    std::vector<::DirectX::XMFLOAT4> m_positions {};  // positions of each particle; m_positions[i] = position vector of the ith particle
    std::vector<::DirectX::XMFLOAT4> m_initial_positions {}; // the starting positions, restored by reset()
    std::vector<::DirectX::XMFLOAT3> m_velocities {}; // velocities of each particle, m_velocities[i] = velocity vector of the ith particle
    std::vector<float> m_densities {}; // stores the calculated density of each particle
    std::size_t m_step_count { 0uz }; // how many steps have run, used to print the density check about once a second

    // define boundaries of a box for bouncing
    DirectX::XMFLOAT3 m_bounds_min { -1.5f, -1.5f, -1.5f };
    DirectX::XMFLOAT3 m_bounds_max {  1.5f,  1.5f,  1.5f };

    float m_damping { 0.5f }; // how much energy a particle loses when bouncing

    // works out the density of every particle from its neighbors' positions
    void _compute_densities();

};



#endif // SIMULATION_HH