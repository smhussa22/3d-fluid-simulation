Texture2D<float> depth_texture : register(t0);

// one big triangle that covers the whole screen, with no vertex buffer
float4 vs_main(uint id : SV_VertexID) : SV_Position
{
    const float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0f, 1.0f);
}

// debug view of the depth texture: black where there is no water, otherwise repeating bands of brightness every 1/8 m of distance, so the bumps are easy to see
float4 ps_main(float4 position : SV_Position) : SV_Target
{
    const float depth = depth_texture.Load(int3(position.xy, 0));

    if (depth <= 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    const float bands = frac(depth * 8.0f);
    return float4(bands, bands, bands, 1.0f);
}
