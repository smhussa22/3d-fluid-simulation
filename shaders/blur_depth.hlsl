Texture2D<float> source : register(t0); // the depth image to smooth (0 where there is no water)

cbuffer BlurParams : register(b3)
{
    float2 direction;       // (1, 0) blurs along x, (0, 1) blurs along y
    float blur_radii;       // how far the blur reaches, in particle radii as they look on screen
    float depth_sigma;      // how quickly the weight falls off with difference in depth, in meters
    float particle_radius;  // world-space radius of one particle, in meters
    float focal_pixels;     // how many pixels one meter spans at a distance of one meter
    float2 padding;
};

// one big triangle that covers the whole screen, with no vertex buffer
float4 vs_main(uint id : SV_VertexID) : SV_Position
{
    const float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0f, 1.0f);
}

// a bilateral blur along one direction: each pixel becomes the weighted average of the depths of pixels in a line through it.
// a neighbor counts more when it is close in the picture (spatial weight) AND close in depth (depth weight), so bumps inside the water smooth out
// but the water's edge, where the depth jumps, stays sharp.
float ps_main(float4 position : SV_Position) : SV_Target
{
    const int2 center_pixel = int2(position.xy);
    const float center_depth = source.Load(int3(center_pixel, 0));

    // no water here, so nothing to smooth
    if (center_depth <= 0.0f)
    {
        return 0.0f;
    }

    // a particle looks smaller the farther away it is, so the blur shrinks with distance and with particle size:
    // reach out blur_radii particle radii as they appear on screen at this depth (at most 48 pixels)
    const float pixel_radius = clamp(blur_radii * particle_radius * focal_pixels / center_depth, 1.0f, 48.0f);
    const int reach = (int)ceil(pixel_radius);
    const float spatial_sigma = max(pixel_radius * 0.5f, 0.5f);

    float weighted_sum = 0.0f;
    float weight_total = 0.0f;

    for (int i = -reach; i <= reach; ++i)
    {
        // Load returns 0 outside the screen, which is skipped below like any other pixel without water
        const float neighbor_depth = source.Load(int3(center_pixel + int2(direction) * i, 0));

        if (neighbor_depth <= 0.0f)
        {
            continue;
        }

        const float depth_difference = neighbor_depth - center_depth;
        const float spatial_weight = exp(-float(i * i) / (2.0f * spatial_sigma * spatial_sigma));
        const float depth_weight = exp(-(depth_difference * depth_difference) / (2.0f * depth_sigma * depth_sigma));
        const float weight = spatial_weight * depth_weight;

        weighted_sum += weight * neighbor_depth;
        weight_total += weight;
    }

    // the center pixel always has weight 1, so this never divides by zero
    return weighted_sum / weight_total;
}
