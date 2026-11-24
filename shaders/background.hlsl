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
    float4 beach;               // the sandy beach: x where it starts rising, tan(slope), x where it levels off, the water level (all 0 = no beach)
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

// the height of the sand at a spot on the ground (beach mode): the beach slope rises from beach.x until it reaches the top level (a little above the water), and
// steep sand banks rise just outside the other three sides of the tank to that same level, so the water sits in a pool and the sand beyond is one flat plain
float ground_height(float2 xz)
{
    const float top = beach.w + 0.15f;
    const float outside = max(max(tank_xz.x - xz.x, 0.0f), max(max(tank_xz.y - xz.y, xz.y - tank_xz.w), 0.0f));
    return floor_y + min(top, max(max(xz.x - beach.x, 0.0f) * beach.y, outside * 6.0f));
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

    // beach mode: march along the ray until it dips under the sand, then narrow the hit down by halving the last step
    if (beach.y > 0.0f)
    {
        float near_t = 0.0f;
        float far_t = -1.0f;

        for (int step_index = 0; step_index < 220; ++step_index)
        {
            const float next_t = near_t + max(0.03f, near_t * 0.02f);
            const float3 point_on_ray = camera_eye.xyz + ray * next_t;

            if (point_on_ray.y < ground_height(point_on_ray.xz))
            {
                far_t = next_t;
                break;
            }

            near_t = next_t;
        }

        // never reached the ground: it is far off, so it is lost in the haze
        if (far_t < 0.0f)
        {
            return float4(sky_color(float3(ray.x, 0.0f, ray.z)), 1.0f);
        }

        for (int refine = 0; refine < 8; ++refine)
        {
            const float middle_t = 0.5f * (near_t + far_t);
            const float3 middle_point = camera_eye.xyz + ray * middle_t;

            if (middle_point.y < ground_height(middle_point.xz))
            {
                far_t = middle_t;
            }
            else
            {
                near_t = middle_t;
            }
        }

        const float3 sand_hit = camera_eye.xyz + ray * far_t;

        // the sand's surface direction from how the height changes to each side
        const float eps = 0.05f;
        const float dx = ground_height(sand_hit.xz + float2(eps, 0.0f)) - ground_height(sand_hit.xz - float2(eps, 0.0f));
        const float dz = ground_height(sand_hit.xz + float2(0.0f, eps)) - ground_height(sand_hit.xz - float2(0.0f, eps));
        const float3 sand_normal = normalize(float3(-dx, 2.0f * eps, -dz));

        // warm sand with a little variation, lit by the sun
        // (the grain fades with distance, where it would only alias)
        const float grain = 1.0f - lerp(0.04f, 0.0f, saturate(far_t / 25.0f)) * (0.5f + 0.5f * sin(sand_hit.x * 23.0f + sin(sand_hit.z * 9.0f) * 2.0f) * sin(sand_hit.z * 31.0f));
        float3 sand_color = float3(0.78f, 0.66f, 0.46f) * grain * (0.45f + 0.65f * saturate(dot(sand_normal, sun_direction)));

        // under the water, light bends into bright moving lines on the sand
        if (sand_hit.x > tank_xz.x && sand_hit.x < tank_xz.z && sand_hit.z > tank_xz.y && sand_hit.z < tank_xz.w && sand_hit.y < floor_y + beach.w)
        {
            sand_color += caustic(sand_hit.xz / 1.5f, time * 0.6f) * float3(0.8f, 0.95f, 1.0f) * 0.4f;
        }

        const float sand_haze = 1.0f - exp(-far_t * 0.01f);
        return float4(lerp(sand_color, sky_color(float3(ray.x, 0.0f, ray.z)), sand_haze), 1.0f);
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
