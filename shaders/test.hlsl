// test shader: writes each thread's index into the buffer, so after a dispatch the buffer should read 0, 1, 2, ...
RWStructuredBuffer<uint> output : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{

    // the last group has spare threads past the end of the buffer, so skip them
    uint count;
    uint stride;
    output.GetDimensions(count, stride);

    if (id.x >= count)
    {

        return;

    }

    output[id.x] = id.x;

}
