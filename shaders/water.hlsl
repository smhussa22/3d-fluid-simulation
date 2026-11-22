#include "camera_common.hlsli"
#include "scene_common.hlsli"

Texture2D<float4> normal_texture : register(t0);     // xyz = surface normal in camera space, w = 1 where there is water
Texture2D<float> thickness_texture : register(t1);   // how much water the view passes through, in meters
Texture2D<float4> background_texture : register(t2); // the scene without water

cbuffer ParticleParams : register(b2)
{
    float particle_radius;
    float3 padding_a;
    float4 light_direction; // xyz = the direction pointing toward the light
    float4 particle_color;  // rgb = base color of the water (used as the deep-water color)
};

cbuffer NormalParams : register(b3)
{
    float2 tan_half_fov;
    float2 screen_size;
};

cbuffer WaterParams : register(b4)
{
    float refraction_strength;  // how far the background shifts behind thick, tilted water, in pixels
    float absorption_strength;  // how quickly water darkens and tints with thickness
    float floor_y;
    float padding_b;
};

// one big triangle that covers the whole screen, with no vertex buffer
float4 vs_main(uint id : SV_VertexID) : SV_Position
{
    const float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0f, 1.0f);
}

// the final picture: the background where there is no water, and where there is water, light that has been bent, absorbed and reflected
float4 ps_main(float4 position : SV_Position) : SV_Target
{
    const int2 pixel = int2(position.xy);
    const float4 surface = normal_texture.Load(int3(pixel, 0));
    const float3 background = background_texture.Load(int3(pixel, 0)).rgb;

    // no water here: just the scene
    if (surface.w <= 0.0f)
    {
        return float4(background, 1.0f);
    }

    const float3 normal = surface.xyz;
    const float thickness = thickness_texture.Load(int3(pixel, 0));

    // the direction of the line of sight through this pixel, in camera space (x right, y up, z forward), pointing from the camera into the scene
    const float2 ndc = float2(position.x / screen_size.x * 2.0f - 1.0f, 1.0f - position.y / screen_size.y * 2.0f);
    const float3 incident = normalize(float3(ndc.x * tan_half_fov.x, ndc.y * tan_half_fov.y, 1.0f));

    // the light and the camera axes, to move directions between camera space and the world
    const float3 forward = -camera_toward.xyz;
    const float3 light_world = normalize(light_direction.xyz);
    const float3 light = float3(dot(light_world, camera_right.xyz), dot(light_world, camera_up.xyz), dot(light_world, forward));

    // refraction: Snell's law (air into water, index 1/1.33) bends the line of sight; how much it moved sideways decides how far we shift where we look up the background,
    // and thicker water shifts it further. screen y points down, camera-space y points up
    const float3 bent = refract(incident, normal, 1.0f / 1.33f);
    const float2 bend = bent.xy - incident.xy;
    const float2 shift = float2(bend.x, -bend.y) * thickness * refraction_strength;
    const int2 shifted_pixel = clamp(pixel + int2(shift), int2(0, 0), int2(screen_size) - 1);
    const float3 seen_through = background_texture.Load(int3(shifted_pixel, 0)).rgb;

    // absorption: water soaks up red fastest, then green, then blue (Beer's law), so deep water goes blue and dark; in its place we add a little light scattered back by the water itself
    const float3 transmittance = exp(-float3(0.45f, 0.12f, 0.05f) * thickness * absorption_strength);
    const float3 scattered = particle_color.rgb * (0.3f + 0.7f * saturate(dot(normal, light) * 0.5f + 0.5f));
    float3 refracted = seen_through * transmittance + scattered * (1.0f - transmittance);

    // a little diffuse shading from the normal so the surface shape reads even in deep water
    refracted *= 0.85f + 0.15f * saturate(dot(normal, light));

    // Fresnel (Schlick's approximation): more mirror-like the more the surface is seen edge-on
    const float cosine = saturate(dot(normal, -incident));
    const float fresnel = 0.02f + 0.98f * pow(1.0f - cosine, 5.0f);

    // what the surface mirrors: the sky in the reflected direction (moved from camera space back to world space)
    const float3 reflected = reflect(incident, normal);
    const float3 reflected_world = camera_right.xyz * reflected.x + camera_up.xyz * reflected.y + forward * reflected.z;
    const float3 mirrored = sky_color(normalize(reflected_world));

    // the sun's glint (Blinn-Phong highlight)
    const float3 halfway = normalize(light - incident);
    const float specular = pow(saturate(dot(normal, halfway)), 160.0f) * 1.2f;

    return float4(lerp(refracted, mirrored, fresnel) + specular, 1.0f);
}
