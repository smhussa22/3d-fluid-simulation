#include "common.hlsli"

StructuredBuffer<float4> positions : register(t0);
StructuredBuffer<float4> velocities : register(t1);
StructuredBuffer<float> densities : register(t2);
StructuredBuffer<float> pressures : register(t3);
StructuredBuffer<uint> cell_start : register(t4);
RWStructuredBuffer<float4> accelerations : register(u0);

// one thread per particle: add up the pressure push and the viscosity drag from every neighbor, then turn the force into an acceleration
[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= particle_count)
    {
        return;
    }

    const uint i = id.x;
    const float3 position = positions[i].xyz;
    const float3 velocity = velocities[i].xyz;
    const int3 center = (int3)cell_coords(position);
    float3 force = float3(0.0f, 0.0f, 0.0f);

    for (int dz = -1; dz <= 1; ++dz)
    {
        const int z = center.z + dz;

        if (z < 0 || z >= (int)cells_z)
        {
            continue;
        }

        for (int dy = -1; dy <= 1; ++dy)
        {
            const int y = center.y + dy;

            if (y < 0 || y >= (int)cells_y)
            {
                continue;
            }

            for (int dx = -1; dx <= 1; ++dx)
            {
                const int x = center.x + dx;

                if (x < 0 || x >= (int)cells_x)
                {
                    continue;
                }

                const uint cell = cell_index(uint3(x, y, z));

                for (uint k = cell_start[cell]; k < cell_start[cell + 1]; ++k)
                {
                    const uint j = k; // the arrays are sorted by cell, so slot k is particle k

                    if (j == i)
                    {
                        continue;
                    }

                    // vector from neighbor j to this particle i
                    const float3 offset = position - positions[j].xyz;
                    const float distance_squared = dot(offset, offset);

                    if (distance_squared >= kernel_radius * kernel_radius)
                    {
                        continue;
                    }

                    const float dist = sqrt(distance_squared);

                    // pressure: push away from the neighbor (skipped when two particles sit on exactly the same spot)
                    if (distance_squared > 0.0f)
                    {
                        const float3 direction = offset / dist;
                        const float pressure_term = particle_mass * (pressures[i] + pressures[j]) / (2.0f * densities[j]) * spiky_gradient(dist);
                        force += pressure_term * direction;
                    }

                    // viscosity: drag this particle's velocity toward the neighbor's
                    const float viscosity_coefficient = viscosity * particle_mass / densities[j] * viscosity_laplacian(dist);
                    force += viscosity_coefficient * (velocities[j].xyz - velocity);
                }
            }
        }
    }

    accelerations[i] = float4(force / densities[i], 0.0f);
}
