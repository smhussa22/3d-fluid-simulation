#include "particle_common.hlsli"

// the depth pass: instead of a color, write how far this pixel's water surface is from the camera (measured along the view direction, in meters)
float ps_main(VertexOutput input, out float depth : SV_Depth) : SV_Target
{
    float3 normal;
    float4 clip;

    // when a particle is discarded due to low density ratio; droplets
    float1 spray_threshold = 100.0;
    if (!sphere_surface(input, normal, clip) || input.density_ratio < 0.3)
    {
        discard;
    }

    // the hardware depth test still picks the nearest sphere at each pixel; clip.w is the distance along the view direction for a perspective projection
    depth = clip.z / clip.w;
    return clip.w;
}
