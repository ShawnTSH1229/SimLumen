#include "SimLumenCommon.hlsl"

#define CARD_TILE_SIZE 8
cbuffer SceneVoxelVisibilityInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

Texture2D<float4> surface_cache_direct_lighting : register(t0);
Texture2D<float4> surface_cache_indirect_lighting : register(t1);
RWTexture2D<float4> surface_cache_combine_lighting : register(u0);

[numthreads(CARD_TILE_SIZE, CARD_TILE_SIZE, 1)]
void LumenSceneCombineLightingCS(uint3 group_idx : SV_GroupID, uint3 group_thread_index : SV_GroupThreadID, uint3 thread_idx: SV_DispatchThreadID)
{
    uint2 card_index_xy = thread_idx.xy / 128;
    uint card_index_1d = card_index_xy.y * card_num_xy + card_index_xy.x;
    if(card_index_1d < scene_card_num)
    {   
        uint2 pixel_pos = thread_idx.xy;
        pixel_pos.y = (SURFACE_CACHE_TEX_SIZE - pixel_pos.y);
        float3 direct_lighting = surface_cache_direct_lighting.Load(int3(pixel_pos,0)).xyz;
        float3 indirect_lighting = surface_cache_indirect_lighting.Load(int3(pixel_pos,0)).xyz;

        surface_cache_combine_lighting[int2(pixel_pos.xy)] = float4(direct_lighting + indirect_lighting, 1.0);
    }
}