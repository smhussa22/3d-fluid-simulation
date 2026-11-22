Texture2D<float> depth_texture : register(t0); // the smoothed depth: distance from the camera in meters, 0 where there is no water

cbuffer NormalParams : register(b3)
{
    float2 tan_half_fov;  // tan(fov / 2) across (x, with the aspect ratio included) and up (y): how far the view spreads sideways per meter of distance
    float2 screen_size;   // the window size in pixels
};

// one big triangle that covers the whole screen, with no vertex buffer
float4 vs_main(uint id : SV_VertexID) : SV_Position
{
    const float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0f, 1.0f);
}

// rebuilds where this pixel's water surface is in 3D, in camera space (x right, y up, z away from the camera), by undoing the perspective projection:
// a pixel's place on screen, from -1 to 1, times how far the view has spread out at that distance, gives the sideways position
float3 view_position(int2 pixel, float depth)
{
    const float2 ndc = float2((float(pixel.x) + 0.5f) / screen_size.x * 2.0f - 1.0f, 1.0f - (float(pixel.y) + 0.5f) / screen_size.y * 2.0f);
    return float3(ndc.x * tan_half_fov.x * depth, ndc.y * tan_half_fov.y * depth, depth);
}

// the surface normal at each water pixel, in camera space: the direction that pixel's piece of surface faces
float4 ps_main(float4 position : SV_Position) : SV_Target
{
    const int2 pixel = int2(position.xy);
    const float center_depth = depth_texture.Load(int3(pixel, 0));

    // no water here
    if (center_depth <= 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    const float3 center = view_position(pixel, center_depth);

    // the four neighbors' depths (0 means no water there, which Load also returns outside the screen)
    const float right_depth = depth_texture.Load(int3(pixel + int2(1, 0), 0));
    const float left_depth = depth_texture.Load(int3(pixel + int2(-1, 0), 0));
    const float up_depth = depth_texture.Load(int3(pixel + int2(0, -1), 0));
    const float down_depth = depth_texture.Load(int3(pixel + int2(0, 1), 0));

    // how much the depth changes toward each neighbor; a missing neighbor counts as infinitely far, so it is never picked
    const float infinity = 1.0e30f;
    const float right_change = right_depth > 0.0f ? abs(right_depth - center_depth) : infinity;
    const float left_change = left_depth > 0.0f ? abs(left_depth - center_depth) : infinity;
    const float up_change = up_depth > 0.0f ? abs(up_depth - center_depth) : infinity;
    const float down_change = down_depth > 0.0f ? abs(down_depth - center_depth) : infinity;

    // with no usable neighbor on a side there is no surface to measure, so face the camera
    if (min(right_change, left_change) == infinity || min(up_change, down_change) == infinity)
    {
        return float4(0.0f, 0.0f, -1.0f, 1.0f);
    }

    // a vector along the surface in the screen-right direction and one in the screen-up direction, each taken toward whichever side is closer in depth
    // (so at the water's edge the vectors stay on the water instead of reaching across to the background or a far layer)
    const float3 along_x = right_change < left_change ? view_position(pixel + int2(1, 0), right_depth) - center : center - view_position(pixel + int2(-1, 0), left_depth);
    const float3 along_y = up_change < down_change ? view_position(pixel + int2(0, -1), up_depth) - center : center - view_position(pixel + int2(0, 1), down_depth);

    // the cross product points straight out of the patch those two vectors span; this order points it toward the camera (-z) for a flat surface
    return float4(normalize(cross(along_y, along_x)), 1.0f);
}
