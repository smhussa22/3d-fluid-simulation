#ifndef SCENE_COMMON_HLSLI
#define SCENE_COMMON_HLSLI

// sun direction
// same direction as light direction (light source); outside the function so the ocean shader can use it too
static const float3 sun_direction = normalize(float3(.4f, .8f, .45f));

// the color of the sky in a world-space direction (normalized): pale at the horizon, deeper blue overhead
float3 sky_color(float3 direction)
{
    const float height = saturate(direction.y);
    // linear interpolation; blends between to values based on param 3
    // in this case, blends between ligth blue and deep blue
    float3 sky = lerp(float3(0.78f, 0.88f, 0.97f), float3(0.18f, 0.42f, 0.85f), sqrt(height));

    // a bright, narrow haze band hugging the horizon (1 at the horizon, gone by about 20 degrees up); the floor fades into this same color, so the horizon reads as a glowing line
    const float band = pow(1.0f - height, 12.0f);
    sky = lerp(sky, float3(1.0f, 0.97f, 0.9f), band * 0.8f);

    // how closely the direction we're looking in aligns with the sun's direction
    // directly towards sun = 1, 45 deg ~0.5, 90 or opposite away = 0, etc.
    const float sun_amount = saturate(dot(direction, sun_direction));

    // disc
    // raise sun amount to teh power of 2000 to create concentratd bright spot, second is rgb color, make it intense by factor of 8
    sky += pow(sun_amount, 2000.0f) * float3(1.0f, 1.0f, 0.7f) * 8.0f;
    // glow
    sky += pow(sun_amount, 12.0f) * float3(1.0f, 0.8f, 0.5f) * 0.3f;
    // calculates how close the current viewing direction is to the horizon
    // lookijg up or down reduces horizon effect, horizontrally maximizes it
    const float horizon = 1.0f - saturate(direction.y);
    // blend sky, orangeish by a factor of how lcose were looking at the sun and horizon
    sky = lerp (sky, float3(1.0f, 0.7f, 0.45f), pow(sun_amount, 6.0f) * horizon * 0.4f);
    return sky;

}

#endif
