#include "particle_common.hlsli"

// the depth pass: instead of a color, write how far this pixel's water surface is from the camera (measured along the view direction, in meters)
float ps_main(VertexOutput input, out float depth : SV_Depth) : SV_Target
{
    float3 normal;
    float4 clip;

    if (!sphere_surface(input, normal, clip))
    {
        discard;
    }

    // the hardware depth test still picks the nearest sphere at each pixel; clip.w is the distance along the view direction for a perspective projection
    depth = clip.z / clip.w;
    return clip.w;
}
