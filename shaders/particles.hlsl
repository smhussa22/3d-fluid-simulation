#include "particle_common.hlsli"

// treat each quad as a sphere: pixels outside the circle are thrown away, the rest get lighting
float4 ps_main(VertexOutput input, out float depth : SV_Depth) : SV_Target
{
    float3 normal;
    float4 clip;

    if (!sphere_surface(input, normal, clip))
    {
        discard;
    }

    // the real depth of that point on the sphere, so overlapping spheres cut into each other correctly instead of showing flat quads
    depth = clip.z / clip.w;

    // brighter where the surface faces the light, plus a small highlight
    const float diffuse = saturate(dot(normal, light_direction.xyz));
    const float3 halfway = normalize(light_direction.xyz + camera_toward.xyz);
    const float specular = pow(saturate(dot(normal, halfway)), 48.0f) * 0.4f;

    return float4(particle_color.rgb * (0.25f + 0.75f * diffuse) + specular, 1.0f);
}
