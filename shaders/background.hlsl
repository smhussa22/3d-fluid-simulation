#include "camera_common.hlsli"
#include "scene_common.hlsli"

cbuffer NormalParams : register(b3)
{
    float2 tan_half_fov;  // how far the view spreads sideways (x, with aspect) and upward (y) per meter of distance
    float2 screen_size;   // the window size in pixels
};

cbuffer WaterParams : register(b4)
{
    float refraction_strength;  // how far the background shifts behind thick, tilted water, in pixels
    float absorption_strength;  // how quickly water darkens and tints with thickness
    float floor_y;              // height of the floor (the bottom of the tank)
    float padding;
};

// one big triangle that covers the whole screen, with no vertex buffer
float4 vs_main(uint id : SV_VertexID) : SV_Position
{
    const float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0f, 1.0f);
}

// the scene behind the water, with no water in it: a checkerboard floor under a sky. each pixel shoots a ray from the camera through that pixel and looks for what it hits
float4 ps_main(float4 position : SV_Position) : SV_Target
{
    const float2 ndc = float2(position.x / screen_size.x * 2.0f - 1.0f, 1.0f - position.y / screen_size.y * 2.0f);
    const float3 forward = -camera_toward.xyz;
    const float3 ray = normalize(forward + camera_right.xyz * (ndc.x * tan_half_fov.x) + camera_up.xyz * (ndc.y * tan_half_fov.y));

    // a ray pointing upward (or level) can only hit the sky
    if (ray.y > -0.0001f)
    {
        return float4(sky_color(ray), 1.0f);
    }

    // where the ray reaches the floor plane
    const float distance_to_floor = (floor_y - camera_eye.y) / ray.y;
    const float3 hit = camera_eye.xyz + ray * distance_to_floor;

    // alternating light and dark tiles, 0.4 m wide
    const float2 tile = floor(hit.xz / 0.4f);
    const float checker = fmod(abs(tile.x + tile.y), 2.0f);
    const float3 floor_color = lerp(float3(0.16f, 0.17f, 0.2f), float3(0.62f, 0.64f, 0.68f), checker);

    // far tiles fade into the haze of the horizon
    const float haze = 1.0f - exp(-distance_to_floor * 0.03f);
    return float4(lerp(floor_color, sky_color(float3(ray.x, 0.0f, ray.z)), haze), 1.0f);
}
