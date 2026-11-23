// the spray pass: particles too sparse to be part of the water surface are drawn as small, soft, white puffs (added onto the finished picture)
// SPRAY_PASS makes vs_main in particle_common.hlsli shrink the droplets and squash the dense particles away
#define SPRAY_PASS
#include "particle_common.hlsli"

// a soft white circle that fades out toward its edge; many overlapping puffs add up (additive blending), so each one is dim
float4 ps_main(VertexOutput input) : SV_Target
{
    const float distance_squared = dot(input.uv, input.uv);

    if (distance_squared > 1.0f)
    {
        discard;
    }

    const float falloff = (1.0f - distance_squared) * (1.0f - distance_squared);
    return float4(1.0f, 1.0f, 1.0f, 1.0f) * falloff * 0.15f;
}
