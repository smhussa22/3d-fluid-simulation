#include "common.hlsli"

StructuredBuffer<float> densities : register(t0);
RWStructuredBuffer<float> pressures : register(u0);

// one thread per particle: pressure grows with how far the density is above the rest density (never negative, so particles only push)
[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= particle_count)
    {
        return;
    }

    pressures[id.x] = max(0.0f, stiffness * (densities[id.x] - rest_density));
}
