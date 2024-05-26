#include "SimLumenCommon.hlsl"

#define CARD_TILE_SIZE 8
cbuffer SceneVoxelVisibilityInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

cbuffer SLumenGlobalConstants : register(b1)
{
    GLOBAL_VIEW_CONSTANT_BUFFER
};

StructuredBuffer<SCardInfo> scene_card_infos : register(t0);
Texture2D<float4> scene_card_albedo : register(t1);
Texture2D<float4> scene_card_normal : register(t2);
Texture2D<float> scene_card_depth : register(t3);

RWTexture2D<float4> surface_cache_direct_lighting : register(u0);

#include "SimLumenSurfaceCacheCommon.hlsl"

[numthreads(CARD_TILE_SIZE, CARD_TILE_SIZE, 1)]
void LumenCardBatchDirectLightingCS(uint3 group_idx : SV_GroupID, uint3 group_thread_index : SV_GroupThreadID, uint3 thread_idx: SV_DispatchThreadID)
{
    uint2 card_index_xy = thread_idx.xy / 128;
    uint card_index_1d = card_index_xy.y * card_num_xy + card_index_xy.x;
    if(card_index_1d < scene_card_num)
    {
        SCardInfo card_info = scene_card_infos[card_index_1d];
        SCardData card_data = GetSurfaceCardData(card_info, float2(uint2(thread_idx.xy % 128u)) / 128.0f, thread_idx.xy);

        float3 light_direction = SunDirection;
        float NoL = dot(light_direction, card_data.world_normal);
        float3 direct_lighting = SunIntensity * NoL;
        surface_cache_direct_lighting[int2(thread_idx.xy)] = float4(direct_lighting,1.0);
    }
}