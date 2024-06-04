#ifndef SCREENSPACE_PROBE_TRACE_COMMON
#define SCREENSPACE_PROBE_TRACE_COMMON 1

#include "SimLumenCommon.hlsl"

void GetScreenProbeTexelRay(uint2 buffer_idx, inout float3 ray_direction)
{
    uint packed_ray_info = structed_is_indirect_table.Load(int3(buffer_idx.xy,0));

    uint2 texel_coord;
    uint level;
    UnpackRayInfo(packed_ray_info, texel_coord, level);

    uint mip_size = 16 >> level;
    float inv_mip_size = 1.0f / float(mip_size);

    float2 probe_uv = (texel_coord + float2(0.5, 0.5)) * inv_mip_size;
    ray_direction = EquiAreaSphericalMapping(probe_uv);
    ray_direction = normalize(ray_direction);
}

float3 GetWorldPosByDepth(float depth, float2 tex_uv)
{
    float4 ndc = float4( tex_uv.xy * 2.0 - 1.0, depth, 1.0f );
    ndc.y = (ndc.y * (-1.0));
	float4 wp = mul(InverseViewProjMatrix, ndc);
	float3 world_position =  wp.xyz / wp.w;
    return world_position;
}
#endif
