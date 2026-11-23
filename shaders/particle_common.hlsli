StructuredBuffer<float4> positions : register(t0);
StructuredBuffer<float> densities : register(t1); // same order as positions
StructuredBuffer<float4> velocities : register(t2); // same order as positions (xyz = velocity in m/s)

#include "camera_common.hlsli"

cbuffer ParticleParams : register(b2)
{
    float particle_radius;  // world-space radius of each drawn sphere
    float spray_threshold;  // density (as a fraction of rest density) below which a particle is spray
    float spray_size;       // radius of a drawn spray droplet, in particle radii
    float thickness_fade;   // water thinner than this many particle diameters fades out
    float4 light_direction; // xyz = the direction pointing toward the light
    float4 particle_color;  // rgb = base color of the water
    float foam_speed;       // particle speed in m/s where foam starts (full foam at twice this)
    float foam_amount;      // how white thin, fast water gets (used by the water shader)
    float2 padding_foam;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;      // position within the quad, from -1 to 1 on each axis
    float3 center : TEXCOORD1;  // the particle's center in the world
    float density_ratio : TEXCOORD2; // the particle's density as a fraction of the rest density (about 1 inside water, lower at the surface, near 0 for stray droplets)
    float speed : TEXCOORD3;    // how fast the particle is moving, in m/s
};

// the four corners of a quad, ordered so a triangle strip made of them faces the camera
static const float2 corners[4] = { float2(-1.0f, -1.0f), float2(-1.0f, 1.0f), float2(1.0f, -1.0f), float2(1.0f, 1.0f) };

// one quad (4 vertices) per particle: SV_InstanceID picks the particle, SV_VertexID picks the corner
VertexOutput vs_main(uint vertex : SV_VertexID, uint instance : SV_InstanceID)
{
    const float3 center = positions[instance].xyz;
    const float2 uv = corners[vertex];

    const float density_ratio = densities[instance] / 1000.0f; // shortcut: rest density hard-coded (GpuSimulation::rest_density), pass it in if it ever changes

#ifdef SPRAY_PASS
    // the spray pass draws only the sparse particles, at the droplet size; dense ones get a zero-size quad that makes no pixels
    const float radius = density_ratio < spray_threshold ? spray_size * particle_radius : 0.0f;
#else
    const float radius = particle_radius;
#endif

    // slide the corner sideways and up along the camera's own axes, so the quad always faces the camera
    const float3 world = center + (camera_right.xyz * uv.x + camera_up.xyz * uv.y) * radius;

    VertexOutput output;
    output.position = mul(float4(world, 1.0f), view_projection);
    output.uv = uv;
    output.center = center;
    output.density_ratio = density_ratio;
    output.speed = length(velocities[instance].xyz);
    return output;
}

// works out where this pixel is on the sphere: returns false for pixels outside the circle, otherwise the surface normal and the clip-space position of that point
// (the normal points from the sphere's center to the point; the last part is how far the point bulges toward the camera)
bool sphere_surface(VertexOutput input, out float3 normal, out float4 clip)
{
    const float distance_squared = dot(input.uv, input.uv);
    normal = float3(0.0f, 0.0f, 0.0f);
    clip = float4(0.0f, 0.0f, 0.0f, 1.0f);

    if (distance_squared > 1.0f)
    {
        return false;
    }

    const float bulge = sqrt(1.0f - distance_squared);
    normal = camera_right.xyz * input.uv.x + camera_up.xyz * input.uv.y + camera_toward.xyz * bulge;
    clip = mul(float4(input.center + normal * particle_radius, 1.0f), view_projection);
    return true;
}
