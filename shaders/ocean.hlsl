#include "camera_common.hlsli"
#include "scene_common.hlsli"

cbuffer OceanParams : register(b5)
{
    float time;           // seconds since start
    float wave_height;    // height of the biggest wave, trough to crest, in meters
    float wavelength;     // distance between crests of the biggest wave, in meters
    float choppiness;     // 0 = rounded waves, 1 = sharp crests

    float wind_angle;     // direction the waves travel, in radians around the up axis
    float patch_extent;   // the sea covers a square this many meters from the middle to each edge
    float grid_cells;     // the sea mesh is this many quads along each side
    float foam_amount;    // how bright the crest foam is

    float foam_threshold; // how pinched the surface must get before it foams
    float3 padding;
};

static const float PI = 3.14159265358979f;
static const float gravity = 9.81f;
static const uint wave_count = 12u;      // waves summed to shade a pixel
static const uint mesh_wave_count = 6u;  // the biggest of them also move the mesh (the rest are too small for the grid to hold)

// each wave travels a little off the wind direction and is a fraction of the biggest wave's length
static const float angle_offsets[12] = { 0.0f, 0.35f, -0.5f, 0.9f, -1.0f, 0.15f, 0.7f, -0.3f, 1.3f, -1.2f, 0.5f, -0.8f };
static const float length_scales[12] = { 1.0f, 0.62f, 0.43f, 0.31f, 0.22f, 0.16f, 0.10f, 0.07f, 0.05f, 0.035f, 0.025f, 0.018f };

struct Wave
{
    float2 direction; // which way it travels on the flat sea (unit length)
    float k;          // wave number: 2 pi / length, so bigger = shorter wave
    float amplitude;  // height from the middle level to a crest, in meters
    float omega;      // how fast the phase advances (deep water: sqrt(g k), so long waves are fast)
    float steepness;  // how far crests lean sideways (0 = plain sine wave, 1 = a sharp peak)
};

Wave make_wave(uint i)
{
    const float angle = wind_angle + angle_offsets[i];
    const float length = wavelength * length_scales[i];

    Wave wave;
    wave.direction = float2(cos(angle), sin(angle));
    wave.k = 2.0f * PI / length;
    wave.amplitude = wave_height * 0.5f * pow(length_scales[i], 1.3f);
    wave.omega = sqrt(gravity * wave.k);

    // the sum of (steepness * k * amplitude) over all waves stays at or below choppiness, so the surface never folds through itself when choppiness <= 1
    wave.steepness = min(1.0f, choppiness / (wave_count * max(wave.k * wave.amplitude, 1e-5f)));
    return wave;
}

// sums Gerstner waves (every point moves in a circle, which sharpens crests and flattens troughs) at a spot on the flat sea:
// displacement = how far the point moves, normal = the surface direction there, jacobian = how squeezed the surface is (below 1 near sharp crests, where foam forms)
void gerstner(float2 p, uint count, out float3 displacement, out float3 normal, out float jacobian)
{
    displacement = float3(0.0f, 0.0f, 0.0f);
    float3 slope = float3(0.0f, 0.0f, 0.0f);
    float dxx = 0.0f;
    float dzz = 0.0f;
    float dxz = 0.0f;

    for (uint i = 0u; i < count; ++i)
    {
        const Wave wave = make_wave(i);
        const float phase = wave.k * dot(wave.direction, p) - wave.omega * time;
        float s;
        float c;
        sincos(phase, s, c);

        const float ka = wave.k * wave.amplitude;
        displacement.x += wave.steepness * wave.amplitude * wave.direction.x * c;
        displacement.z += wave.steepness * wave.amplitude * wave.direction.y * c;
        displacement.y += wave.amplitude * s;

        slope.x -= wave.direction.x * ka * c;
        slope.z -= wave.direction.y * ka * c;
        slope.y += wave.steepness * ka * s;

        dxx -= wave.steepness * ka * wave.direction.x * wave.direction.x * s;
        dzz -= wave.steepness * ka * wave.direction.y * wave.direction.y * s;
        dxz -= wave.steepness * ka * wave.direction.x * wave.direction.y * s;
    }

    normal = normalize(float3(slope.x, 1.0f - slope.y, slope.z));
    jacobian = (1.0f + dxx) * (1.0f + dzz) - dxz * dxz;
}

struct VertexOutput
{
    float4 position : SV_Position;
    float2 parameter : TEXCOORD0; // where this point sits on the flat sea, before any wave moves it
    float3 world : TEXCOORD1;     // where it is in the world
};

// the two triangles of a quad, as offsets of its corners in grid steps; the winding is clockwise seen from above, which is what the GPU treats as front-facing
static const uint2 corner_offsets[6] = { uint2(0, 0), uint2(0, 1), uint2(1, 0), uint2(1, 0), uint2(0, 1), uint2(1, 1) };

// the sea is a grid of quads with no vertex buffer: SV_VertexID says which quad (6 vertices each) and which corner of it
VertexOutput vs_main(uint id : SV_VertexID)
{
    const uint cells = (uint)grid_cells;
    const uint quad = id / 6u;
    const uint2 grid = uint2(quad % cells, quad / cells) + corner_offsets[id % 6u];

    // grid steps run from -1 to 1; squaring them packs the quads close together in the middle and spreads them out toward the horizon
    const float2 u = float2(grid) / grid_cells * 2.0f - 1.0f;
    const float2 parameter = sign(u) * (0.15f * abs(u) + 0.85f * u * u) * patch_extent;

    // move the point with the big waves
    float3 displacement;
    float3 normal;
    float jacobian;
    gerstner(parameter, mesh_wave_count, displacement, normal, jacobian);

    VertexOutput output;
    output.parameter = parameter;
    output.world = float3(parameter.x + displacement.x, displacement.y, parameter.y + displacement.z);
    output.position = mul(float4(output.world, 1.0f), view_projection);
    return output;
}

float4 ps_main(VertexOutput input) : SV_Target
{
    // the surface direction at this pixel from all the waves, small ones included
    float3 displacement;
    float3 normal;
    float jacobian;
    gerstner(input.parameter, wave_count, displacement, normal, jacobian);

    // the direction from this point to the camera; far away, the smallest waves are less than a pixel wide and would sparkle and shimmer, so the surface is flattened with distance
    const float3 to_eye = camera_eye.xyz - input.world;
    const float distance = length(to_eye);
    const float3 view = to_eye / distance;
    normal = normalize(lerp(normal, float3(0.0f, 1.0f, 0.0f), 0.85f * smoothstep(80.0f, 500.0f, distance)));

    // Fresnel (Schlick's approximation): looking straight down you see into the water, looking along the surface you see the sky mirrored
    const float cosine = saturate(dot(normal, view));
    const float fresnel = 0.02f + 0.98f * pow(1.0f - cosine, 5.0f);

    // what the surface mirrors: the sky in the reflected direction (flipped up if a steep wave points it below the horizon), plus a wider sun streak
    float3 reflected = reflect(-view, normal);
    reflected.y = abs(reflected.y);
    const float3 mirrored = sky_color(reflected) + pow(saturate(dot(reflected, sun_direction)), 150.0f) * 0.8f * float3(1.0f, 0.85f, 0.6f);

    // the water itself: dark blue in the troughs, greener and lighter on the crests where the light shines through thinner water
    const float crest = saturate(input.world.y / max(wave_height, 0.1f) * 0.8f + 0.35f);
    const float3 body = lerp(float3(0.01f, 0.06f, 0.10f), float3(0.06f, 0.30f, 0.30f), crest) * (0.4f + 0.6f * saturate(dot(normal, sun_direction)));

    // crest foam: where the surface is squeezed together (jacobian below the threshold) the water piles up and breaks, so it turns white; the foam travels with the crests
    const float foam = saturate((foam_threshold - jacobian) / 0.25f) * foam_amount;
    const float3 foam_color = float3(0.92f, 0.96f, 1.0f) * (0.55f + 0.45f * saturate(dot(normal, sun_direction)));
    const float3 surface = lerp(lerp(body, mirrored, fresnel), foam_color, saturate(foam));

    // distance haze: far water fades into the same color the horizon has, which also hides the edge of the sea
    const float haze = 1.0f - exp(-pow(distance / 600.0f, 2.0f));
    const float3 haze_color = sky_color(normalize(float3(-view.x, 0.0f, -view.z)));

    return float4(lerp(surface, haze_color, haze), 1.0f);
}
