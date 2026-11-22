StructuredBuffer<float4> positions : register(t0);

#include "camera_common.hlsli"

cbuffer ParticleParams : register(b2)
{
    float particle_radius; // world-space radius of each drawn sphere
    float3 padding;
    float4 light_direction; // xyz = the direction pointing toward the light
    float4 particle_color;  // rgb = base color of the water
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;      // position within the quad, from -1 to 1 on each axis
    float3 center : TEXCOORD1;  // the particle's center in the world
};

// the four corners of a quad, ordered so a triangle strip made of them faces the camera
static const float2 corners[4] = { float2(-1.0f, -1.0f), float2(-1.0f, 1.0f), float2(1.0f, -1.0f), float2(1.0f, 1.0f) };

// one quad (4 vertices) per particle: SV_InstanceID picks the particle, SV_VertexID picks the corner
VertexOutput vs_main(uint vertex : SV_VertexID, uint instance : SV_InstanceID)
{
    const float3 center = positions[instance].xyz;
    const float2 uv = corners[vertex];

    // slide the corner sideways and up along the camera's own axes, so the quad always faces the camera
    const float3 world = center + (camera_right.xyz * uv.x + camera_up.xyz * uv.y) * particle_radius;

    VertexOutput output;
    output.position = mul(float4(world, 1.0f), view_projection);
    output.uv = uv;
    output.center = center;
    return output;
}

// works out where this pixel is on the sphere: returns false for pixels outside the circle, otherwise the surface normal and the clip-space position of that point
// (the normal points from the sphere's center to the point; the last part is how far the point bulges toward the camera)
bool sphere_surface(VertexOutput input, out float3 normal, out float4 clip)
{
    const float distance_squared = dot(input.uv, input.uv);
    normal = float3(0.0f, 0.0f, 0.0f);
    clip = float4(0.0f, 0.0f, 0.0f, 1.0f);

    if (distance_squared > 1.0f)
    {
        return false;
    }

    const float bulge = sqrt(1.0f - distance_squared);
    normal = camera_right.xyz * input.uv.x + camera_up.xyz * input.uv.y + camera_toward.xyz * bulge;
    clip = mul(float4(input.center + normal * particle_radius, 1.0f), view_projection);
    return true;
}
