#include "common.hlsli"

StructuredBuffer<uint> cell_counts : register(t0);
RWStructuredBuffer<uint> cell_start : register(u0); // cell_start[c] = how many particles are in all earlier cells (one extra entry marks the end)

groupshared uint chunk_totals[1024]; // total particle count of each thread's chunk of cells

// one group of 1024 threads: each thread totals a chunk of cells, the chunk totals become starting offsets, then each thread fills in its chunk
[numthreads(1024, 1, 1)]
void main(uint3 thread : SV_GroupThreadID)
{
    const uint chunk_size = (cell_count + 1023) / 1024;
    const uint begin = min(thread.x * chunk_size, cell_count);
    const uint end = min(begin + chunk_size, cell_count);

    // total this thread's chunk
    uint total = 0;

    for (uint total_cell = begin; total_cell < end; ++total_cell)
    {
        total += cell_counts[total_cell];
    }

    chunk_totals[thread.x] = total;
    GroupMemoryBarrierWithGroupSync();

    // thread 0 turns the chunk totals into where each chunk starts (and records the grand total in the extra entry)
    if (thread.x == 0)
    {
        uint running = 0;

        for (uint t = 0; t < 1024; ++t)
        {
            const uint chunk_total = chunk_totals[t];
            chunk_totals[t] = running;
            running += chunk_total;
        }

        cell_start[cell_count] = running;
    }

    GroupMemoryBarrierWithGroupSync();

    // each thread writes the running total for the cells in its chunk
    uint running = chunk_totals[thread.x];

    for (uint c = begin; c < end; ++c)
    {
        cell_start[c] = running;
        running += cell_counts[c];
    }
}
