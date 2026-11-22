#include "common.hlsli"

StructuredBuffer<uint> particle_cell : register(t0);
StructuredBuffer<uint> cell_start : register(t1);
RWStructuredBuffer<uint> cell_fill : register(u0);       // next free slot per cell
RWStructuredBuffer<uint> sorted_indices : register(u1);  // particle indices grouped by cell

// one thread per particle: claim the next free slot in its cell (atomically) and write its index there
[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= particle_count)
    {
        return;
    }

    const uint cell = particle_cell[id.x];
    uint slot;
    InterlockedAdd(cell_fill[cell], 1, slot);
    sorted_indices[cell_start[cell] + slot] = id.x;
}
