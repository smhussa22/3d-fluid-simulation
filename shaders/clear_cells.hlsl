#include "common.hlsli"

RWStructuredBuffer<uint> cell_counts : register(u0); // particles per cell
RWStructuredBuffer<uint> cell_fill : register(u1);   // next free slot per cell while sorting

// one thread per grid cell: reset both counters to zero
[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= cell_count)
    {
        return;
    }

    cell_counts[id.x] = 0;
    cell_fill[id.x] = 0;
}
