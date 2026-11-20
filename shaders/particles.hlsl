StructuredBuffer<float4> positions : register(t0);

cbuffer Camera : register(b0)
{
    row_major float4x4 view_projection;
};

float4 vs_main(uint id : SV_VertexID) : SV_Position
{
    return mul(float4(positions[id].xyz, 1.0f), view_projection);
}

float4 ps_main() : SV_Target
{
    return float4(0.3f, 0.7f, 1.0f, 1.0f);
}
