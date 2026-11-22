#ifndef SCENE_COMMON_HLSLI
#define SCENE_COMMON_HLSLI

// the color of the sky in a world-space direction (normalized): pale at the horizon, deeper blue overhead
float3 sky_color(float3 direction)
{
    const float height = saturate(direction.y);
    return lerp(float3(0.78f, 0.88f, 0.97f), float3(0.18f, 0.42f, 0.85f), sqrt(height));
}

#endif
