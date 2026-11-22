#include "common.hlsli"

StructuredBuffer<uint> sorted_indices : register(t0);        // sorted_indices[k] = which old particle belongs in slot k
StructuredBuffer<float4> positions : register(t1);           // old order
StructuredBuffer<float4> velocities : register(t2);          // old order
RWStructuredBuffer<float4> positions_sorted : register(u0);  // new order: grouped by grid cell
RWStructuredBuffer<float4> velocities_sorted : register(u1); // new order: grouped by grid cell

// one thread per slot: copy that slot's particle into its new place, so particles in the same cell end up next to each other in memory
[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= particle_count)
    {
        return;
    }

    const uint old_index = sorted_indices[id.x];
    positions_sorted[id.x] = positions[old_index];
    velocities_sorted[id.x] = velocities[old_index];
}
