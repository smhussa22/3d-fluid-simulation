#ifndef SIMULATION_HH
#define SIMULATION_HH

#include <DirectXMath.h>

#include <vector>

class Simulation
{
public:

    // values that can be changed while the simulation runs, such as from sliders
    struct Settings
    {

        float gravity { -9.8f };    // gravity acceleration in m/s^2 (negative = downward)
        float stiffness { 150.0f };   // how strongly the fluid resists being squeezed
        float viscosity { 5.0f };   // how strongly neighboring particles drag each other toward the same velocity; higher = thicker, calmer fluid
        float damping { 0.5f };     // how much speed a particle keeps (reversed) when it bounces off a wall

    };

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

    // the live-tunable settings, so a UI can read and change them
    Settings& settings();

    // switches between the grid neighbor search (true) and checking every pair (false); used to compare speed and results
    void setUseGrid(bool use_grid);

    // current particle positions (w = 1), laid out so they can be uploaded straight to the GPU buffer
    const std::vector<::DirectX::XMFLOAT4>& positions() const;

private:
    
    static constexpr float kernel_radius { 0.2f };   // maximum distance at which two particles influence each other (SPH smoothing radius)
    
    // each particle's mass; 1.0 kg because particles are spaced 0.1m apart in each dimension, representing 0.001m^3 of water at 1000 kg/m^3
    static constexpr float particle_mass { 1.0f };   
    
    static constexpr float rest_density { 1000.0f }; // water has a density of 1000 kg per cubed metre



    static constexpr float cell_size { kernel_radius }; // width of one grid cell; equal to the kernel radius so every neighbor is in the particle's cell or one of the 26 around it

    std::vector<::DirectX::XMFLOAT4> m_positions {};  // positions of each particle; m_positions[i] = position vector of the ith particle
    std::vector<::DirectX::XMFLOAT4> m_initial_positions {}; // the starting positions, restored by reset()
    std::vector<::DirectX::XMFLOAT3> m_velocities {}; // velocities of each particle, m_velocities[i] = velocity vector of the ith particle
    
    std::vector<float> m_densities {}; // stores the calculated density of each particle
    
    std::vector<float> m_pressures {}; // stores the calculated pressure of each particle
    std::vector<::DirectX::XMFLOAT3> m_accelerations {}; // stores the pressure-induced acceleration of each particle

    std::size_t m_step_count { 0uz }; // how many steps have run, used to print the density check about once a second

    // define boundaries of a box for bouncing
    DirectX::XMFLOAT3 m_bounds_min { -0.6f, -0.6f, -0.6f };
    DirectX::XMFLOAT3 m_bounds_max {  0.6f,  0.6f,  0.6f };

    Settings m_settings {}; // the live-tunable values (changed from the controls panel)

    // the neighbor search grid: the tank is cut into cells, and particles are sorted by cell
    int m_cells_x { 1 }; // number of grid cells along x
    int m_cells_y { 1 }; // number of grid cells along y
    int m_cells_z { 1 }; // number of grid cells along z
    std::size_t m_cell_count { 1uz }; // total number of grid cells
    std::vector<std::size_t> m_particle_cell {}; // m_particle_cell[i] = index of the cell particle i is in
    std::vector<std::size_t> m_cell_start {};    // m_cell_start[c] = where cell c's particles begin in m_sorted_indices (one extra entry marks the end)
    std::vector<std::size_t> m_sorted_indices {}; // particle indices grouped by cell
    bool m_use_grid { true }; // whether neighbor searches use the grid or check every pair

    // works out the density of every particle from its neighbors' positions
    void _compute_densities();

    // calculates the pressure of every particle using its previously calculated density.
    void _compute_pressures();

    // calculates the pressure-induced acceleration of each particle from its neighbors
    void _compute_pressure_forces();

    // adds the viscosity-induced acceleration to each particle, from the velocity differences with its neighbors; run after _compute_pressure_forces
    void _compute_viscosity_forces();

    // which grid cell (x, y, z) a position falls in, clamped to the grid so a particle on the tank wall can't index outside it
    ::DirectX::XMINT3 _cell_coords(const ::DirectX::XMFLOAT4& position) const;

    // sorts the particles into grid cells; must be redone whenever the particles move
    void _build_grid();

    // calls function(j) for every particle j that could be within the kernel radius of particle i (including i itself)
    template <typename Function>
    void _for_each_neighbor(std::size_t i, Function&& function) const;

};



#endif // SIMULATION_HH