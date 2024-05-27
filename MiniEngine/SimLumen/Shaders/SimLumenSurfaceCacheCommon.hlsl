#ifndef SIMLUMEN_SURFACE_CACHE_COMMON
#define SIMLUMEN_SURFACE_CACHE_COMMON

#include "SimLumenCommon.hlsl"

SCardData GetSurfaceCardData(SCardInfo card_info, float2 card_uv, uint2 atlas_pos)
{
    SCardData card_data = (SCardData)0;
    card_data.is_valid = false;
    float depth = scene_card_depth.Load(int3(atlas_pos,0));
    if(depth != 0.0f)
    {
        card_data.is_valid = true;
        float3 normal = scene_card_normal.Load(int3(atlas_pos,0)).xyz * 2.0 - 1.0;
        float3 world_normal = mul((float3x3)card_info.local_to_world, normal);
        card_data.world_normal = world_normal;
        card_data.albedo = scene_card_albedo.Load(int3(atlas_pos,0)).xyz;

        float3 local_position;
        local_position.xy = (card_uv * (2.0f) - 1.0f) * card_info.rotated_extents.xy;
        local_position.z = -(depth * 2.0 - 1.0f)  * card_info.rotated_extents.z;
        float3 rotate_back_pos = mul((float3x3)card_info.rotate_back_matrix, local_position);
        rotate_back_pos += card_info.bound_center;;

        float3 world_position = mul(card_info.local_to_world, float4(rotate_back_pos,1.0)).xyz;
        card_data.world_position = world_position;
    }
    return card_data;
};
#endif