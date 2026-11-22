#include "common.hlsli"

StructuredBuffer<float4> positions : register(t0);
RWStructuredBuffer<uint> particle_cell : register(u0); // which cell each particle is in
RWStructuredBuffer<uint> cell_counts : register(u1);   // particles per cell

// one thread per particle: find its cell, and add 1 to that cell's count (atomically, since many threads can hit the same cell)
[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= particle_count)
    {
        return;
    }

    const uint cell = cell_index(cell_coords(positions[id.x].xyz));
    particle_cell[id.x] = cell;
    InterlockedAdd(cell_counts[cell], 1);
}
