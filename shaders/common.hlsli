// shared by every simulation compute pass: the constants, grid helpers and SPH kernels
// the cbuffer layout must match GpuSimulation::Params in GpuSimulation.hh exactly (same order, 16 bytes per row)

cbuffer Params : register(b0)
{
    uint particle_count;     // number of particles
    float delta_time;        // seconds this step advances
    float kernel_radius;     // SPH smoothing radius
    float particle_mass;     // mass of one particle

    float rest_density;      // density the fluid wants to be at
    float stiffness;         // how strongly the fluid resists being squeezed
    float viscosity;         // how strongly neighbors drag each other toward the same velocity
    float damping;           // speed a particle keeps (reversed) when it bounces off a wall

    float gravity;           // gravity acceleration in m/s^2 (negative = downward)
    float cell_size;         // width of one grid cell
    uint cells_x;            // grid cells along x
    uint cells_y;            // grid cells along y

    uint cells_z;            // grid cells along z
    uint cell_count;         // total grid cells
    float poly6_coefficient; // 315 / (64 pi h^9), worked out once on the CPU
    float spiky_coefficient; // 45 / (pi h^6), shared by the pressure and viscosity kernels

    float4 bounds_min;       // tank minimum corner (w unused)
    float4 bounds_max;       // tank maximum corner (w unused)

    float4 beach;            // x = where the sloped floor starts along x, y = sin(slope angle), z = cos(slope angle), w = 1 if there is a beach
    float4 paddle;           // x = where the wave-maker paddle's face is along x, y = how fast it is moving along x (zw unused)
};

static const float PI = 3.14159265358979f;

// which grid cell (x, y, z) a position falls in, clamped to the grid so a particle on the tank wall can't index outside it
uint3 cell_coords(float3 position)
{
    int3 cell = (int3)floor((position - bounds_min.xyz) / cell_size);
    cell = clamp(cell, int3(0, 0, 0), int3((int)cells_x - 1, (int)cells_y - 1, (int)cells_z - 1));
    return (uint3)cell;
}

// the single number that names a grid cell
uint cell_index(uint3 cell)
{
    return cell.x + cell.y * cells_x + cell.z * cells_x * cells_y;
}

// how much a neighbor contributes to a particle's density, from the squared distance between them (0 beyond the kernel radius)
float poly6(float distance_squared)
{
    const float radius_squared = kernel_radius * kernel_radius;

    if (distance_squared >= radius_squared)
    {
        return 0.0f;
    }

    const float diff = radius_squared - distance_squared;
    return poly6_coefficient * diff * diff * diff;
}

// magnitude of the pressure kernel's gradient at a distance (0 beyond the kernel radius)
float spiky_gradient(float distance)
{
    const float radius_squared = kernel_radius * kernel_radius;
    if (distance >= kernel_radius)
    {
        return 0.0f;
    }

    const float diff = kernel_radius - distance;
    return spiky_coefficient * diff * diff;
}

// the viscosity kernel's Laplacian at a distance (0 beyond the kernel radius)
float viscosity_laplacian(float distance)
{
    const float radius_squared = kernel_radius * kernel_radius;
    if (distance >= kernel_radius)
    {
        return 0.0f;
    }

    return spiky_coefficient * (kernel_radius - distance);
}
