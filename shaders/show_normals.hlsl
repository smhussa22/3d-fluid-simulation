Texture2D<float4> normal_texture : register(t0); // xyz = normal in camera space, w = 1 where there is water

// one big triangle that covers the whole screen, with no vertex buffer
float4 vs_main(uint id : SV_VertexID) : SV_Position
{
    const float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, 0.0f, 1.0f);
}

// debug view of the normals: each direction as a color (x -> red, y -> green, z -> blue, each shifted from -1..1 to 0..1), black where there is no water
float4 ps_main(float4 position : SV_Position) : SV_Target
{
    const float4 normal = normal_texture.Load(int3(position.xy, 0));

    if (normal.w <= 0.0f)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    return float4(normal.xyz * 0.5f + 0.5f, 1.0f);
}
