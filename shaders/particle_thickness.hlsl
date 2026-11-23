#include "particle_common.hlsli"

// the thickness pass: each sphere adds the length of the line of sight that passes through it (blending adds these up),
// so each pixel ends up with roughly how much water the view looks through there
// x = that thickness; y = the same thickness scaled by how fast the particle moves (foam), so y / x is the average foam amount at the pixel
float2 ps_main(VertexOutput input) : SV_Target
{
    float3 normal;
    float4 clip;

    if (!sphere_surface(input, normal, clip))
    {
        discard;
    }

    // the line through a sphere is longest through the middle: twice the radius times how far the front point bulges toward the camera
    const float bulge = dot(normal, camera_toward.xyz);
    const float thickness = 2.0f * particle_radius * bulge;

    // 0 below foam_speed, rising to 1 at twice foam_speed
    const float foam = saturate((input.speed - foam_speed) / foam_speed);

    return float2(thickness, thickness * foam);
}
