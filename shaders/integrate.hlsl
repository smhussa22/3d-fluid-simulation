#include "common.hlsli"

StructuredBuffer<float4> accelerations : register(t0);  // pressure + viscosity acceleration from the forces pass
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

    // work on local copies of this particle's data, written back at the end
    float3 position = positions[id.x].xyz;
    float3 velocity = velocities[id.x].xyz;

    // semi-implicit Euler: update the velocity first, then move using the new velocity
    velocity += accelerations[id.x].xyz * delta_time;
    velocity.y += gravity * delta_time;
    position += velocity * delta_time;

    // bounce off the tank walls: put the particle back on the wall and reverse (and weaken) its velocity along that axis
    if (position.x < bounds_min.x)
    {
        position.x = bounds_min.x;
        velocity.x *= -damping;
    }

    if (position.x > bounds_max.x)
    {
        position.x = bounds_max.x;
        velocity.x *= -damping;
    }

    if (position.y < bounds_min.y)
    {
        position.y = bounds_min.y;
        velocity.y *= -damping;
    }

    if (position.y > bounds_max.y)
    {
        position.y = bounds_max.y;
        velocity.y *= -damping;
    }

    if (position.z < bounds_min.z)
    {
        position.z = bounds_min.z;
        velocity.z *= -damping;
    }

    if (position.z > bounds_max.z)
    {
        position.z = bounds_max.z;
        velocity.z *= -damping;
    }

    positions[id.x] = float4(position, 1.0f);
    velocities[id.x] = float4(velocity, 0.0f);
}
