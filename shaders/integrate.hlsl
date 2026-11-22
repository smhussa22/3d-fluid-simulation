#include "common.hlsli"

RWStructuredBuffer<float4> positions : register(u0);  // particle positions (the renderer reads this too)
RWStructuredBuffer<float4> velocities : register(u1); // particle velocities (w unused)

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    // the last group has spare threads past the end of the buffer
    if (id.x >= particle_count)
    {
        return;
    }

    // your turn: apply gravity, move the particle, and bounce it off the tank walls
    // (positions[id.x], velocities[id.x], gravity, delta_time, bounds_min, bounds_max and damping are all available)
}
