#include "common.hlsli"

StructuredBuffer<float4> positions : register(t0);
StructuredBuffer<uint> cell_start : register(t1);
RWStructuredBuffer<float> densities : register(u0);

// one thread per particle (the arrays are already sorted by cell, so slot k in a cell is just positions[k]): sum the kernel contributions of every particle in the 27 surrounding cells (including itself)
[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= particle_count)
    {
        return;
    }

    const float3 position = positions[id.x].xyz;
    const int3 center = (int3)cell_coords(position);
    float density = 0.0f;

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
                    const float3 offset = position - positions[k].xyz;
                    density += particle_mass * poly6(dot(offset, offset));
                }
            }
        }
    }

    densities[id.x] = density;
}
