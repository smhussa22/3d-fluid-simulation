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

    VertexOutput output;
    output.parameter = parameter;
    output.world = float3(parameter.x, 0.0f, parameter.y);
    output.position = mul(float4(output.world, 1.0f), view_projection);
    return output;
}

float4 ps_main(VertexOutput input) : SV_Target
{
    // distance haze: far water fades into the same color the horizon has, which also hides the edge of the sea
    const float3 to_point = input.world - camera_eye.xyz;
    const float distance = length(to_point);
    const float haze = 1.0f - exp(-pow(distance / 450.0f, 2.0f));
    const float3 haze_color = sky_color(normalize(float3(to_point.x, 0.0f, to_point.z)));

    return float4(lerp(float3(0.02f, 0.1f, 0.16f), haze_color, haze), 1.0f);
}
