#ifndef SIMLUMEN_SURFACE_CACHE_COMMON
#define SIMLUMEN_SURFACE_CACHE_COMMON

#include "SimLumenCommon.hlsl"

SCardData GetSurfaceCardData(SCardInfo card_info, float2 card_uv, uint2 atlas_pos)
{
    SCardData crad_data = (SCardData)0;
    crad_data.is_valid = false;
    float depth = scene_card_depth.Load(int3(atlas_pos,0));
    if(depth != 0.0f)
    {
        crad_data.is_valid = true;
        float3 normal = scene_card_normal.Load(int3(atlas_pos,0)).xyz;
        float3 world_normal = mul((float3x3)card_info.local_to_world, normal);
        crad_data.world_normal = world_normal;
        crad_data.albedo = scene_card_albedo.Load(int3(atlas_pos,0)).xyz;

        float3 local_position;
        local_position.xy = (card_uv * (2.0f) - 1.0f) * card_info.rotated_extents.xy;
        local_position.z = -(depth * 2.0 - 1.0f)  * card_info.rotated_extents.z;
        float3 world_position = mul((float3x3)card_info.local_to_world, local_position);
        crad_data.world_position = world_position;
    }
    return crad_data;
};
#endif