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
    float time;                 // seconds since start
    float4 tank_xz;             // the tank's footprint: min x, min z, max x, max z
};

// a cheap animated caustic pattern: the bright, wobbling lines light makes on the floor after bending through a rippling surface
// it adds up a few layers of swirling waves and keeps only where they pile up; the result is near 0 in most places and bright along thin lines
float caustic(float2 uv, float t)
{
    const float tau = 6.28318530718f;
    float2 p = uv * tau;
    p = p - tau * floor(p / tau) - 250.0f;
    float2 i = p;
    float brightness = 1.0f;
    const float intensity = 0.005f;

    for (int n = 0; n < 5; ++n)
    {
        const float layer_time = t * (1.0f - 3.5f / float(n + 1));
        i = p + float2(cos(layer_time - i.x) + sin(layer_time + i.y), sin(layer_time - i.y) + cos(layer_time + i.x));
        brightness += 1.0f / length(float2(p.x / (sin(i.x + layer_time) / intensity), p.y / (cos(i.y + layer_time) / intensity)));
    }

    brightness /= 5.0f;
    brightness = 1.17f - pow(brightness, 1.4f);
    return pow(abs(brightness), 8.0f);
}

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
    float checker = fmod(abs(tile.x + tile.y), 2.0f);

    // far away, a tile is smaller than a pixel and the pattern aliases into rings: fade to the average gray once one pixel covers a good part of a tile
    // (a pixel's footprint on the floor grows with distance, and stretches out at grazing angles)
    const float footprint = distance_to_floor * (2.0f * tan_half_fov.y / screen_size.y) / max(-ray.y, 0.02f);
    checker = lerp(checker, 0.5f, smoothstep(0.1f, 0.35f, footprint));
    float3 floor_color = lerp(float3(0.16f, 0.17f, 0.2f), float3(0.62f, 0.64f, 0.68f), checker);

    // under the tank, the water above throws moving bright lines onto the floor
    // (shortcut: drawn wherever the tank floor is, even where a splash has left no water above it)
    if (hit.x > tank_xz.x && hit.x < tank_xz.z && hit.z > tank_xz.y && hit.z < tank_xz.w)
    {
        floor_color += caustic(hit.xz / 1.5f, time * 0.6f) * float3(0.8f, 0.95f, 1.0f) * 0.7f;
    }

    // far tiles fade into the haze of the horizon
    const float haze = 1.0f - exp(-distance_to_floor * 0.03f);
    return float4(lerp(floor_color, sky_color(float3(ray.x, 0.0f, ray.z)), haze), 1.0f);
}
