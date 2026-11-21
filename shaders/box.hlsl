cbuffer Camera : register(b0)
{
    row_major float4x4 view_projection;
};

cbuffer Box : register(b1)
{
    float4 box_min;
    float4 box_max;
};

// the 12 edges of a box as pairs of corner numbers; a corner's bits pick min/max: bit 0 = x, bit 1 = y, bit 2 = z
static const uint2 edges[12] =
{
    uint2(0, 1), uint2(1, 3), uint2(3, 2), uint2(2, 0),
    uint2(4, 5), uint2(5, 7), uint2(7, 6), uint2(6, 4),
    uint2(0, 4), uint2(1, 5), uint2(2, 6), uint2(3, 7)
};

float4 vs_main(uint id : SV_VertexID) : SV_Position
{
    uint2 edge = edges[id / 2];
    uint corner = (id % 2 == 0) ? edge.x : edge.y;

    float3 position = float3(
        (corner & 1) ? box_max.x : box_min.x,
        (corner & 2) ? box_max.y : box_min.y,
        (corner & 4) ? box_max.z : box_min.z);

    return mul(float4(position, 1.0f), view_projection);
}

float4 ps_main() : SV_Target
{
    return float4(0.6f, 0.6f, 0.65f, 1.0f);
}
