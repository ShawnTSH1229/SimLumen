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
Texture2D<float> shadow_depth_buffer : register(t4);

RWTexture2D<float4> surface_cache_direct_lighting : register(u0);

#include "SimLumenSurfaceCacheCommon.hlsl"

[numthreads(CARD_TILE_SIZE, CARD_TILE_SIZE, 1)]
void LumenCardBatchDirectLightingCS(uint3 group_idx : SV_GroupID, uint3 group_thread_index : SV_GroupThreadID, uint3 thread_idx: SV_DispatchThreadID)
{
    uint2 card_index_xy = thread_idx.xy / 128;
    uint card_index_1d = card_index_xy.y * card_num_xy + card_index_xy.x;
    if(card_index_1d < scene_card_num)
    {   
        uint2 pixel_pos = thread_idx.xy;
        pixel_pos.y = (SURFACE_CACHE_TEX_SIZE - pixel_pos.y);

        SCardInfo card_info = scene_card_infos[card_index_1d];
        SCardData card_data = GetSurfaceCardData(card_info, float2(uint2(thread_idx.xy % 128u)) / 128.0f, pixel_pos.xy);

        float shadow = 0.0;
        {
            float4 shadow_screen_pos = mul(ShadowViewProjMatrix, float4(card_data.world_position,1.0));
            float2 shadow_uv = shadow_screen_pos.xy;
            shadow_uv = shadow_uv * float2(0.5, -0.5) + float2(0.5, 0.5);
            float2 shadow_pixel_pos = shadow_uv.xy * 2048;

            float shadow_depth_value = shadow_depth_buffer.Load(int3(shadow_pixel_pos.xy,0)).x;;
            shadow = ((shadow_screen_pos.z + 0.0005) < shadow_depth_value ) ? 0.0 :1.0;
        }

        //directional lighting
        float3 directional_lighting = float3(0,0,0);
        {
            float3 light_direction = SunDirection;
            float NoL = saturate(dot(light_direction, card_data.world_normal));
            directional_lighting = SunIntensity * NoL * card_data.albedo * shadow * (1.0 / PI);
        }

        float3 point_lighting = float3(0,0,0);
        {
            float3 point_light_direction = point_light_world_pos - card_data.world_position;
            float light_dist = length(point_light_direction);
            float attenuation = saturate((point_light_radius - light_dist) / point_light_radius);   
            float NoL = saturate(dot(normalize(point_light_direction), card_data.world_normal));
            point_lighting = NoL * card_data.albedo * attenuation * attenuation * (1.0 / PI);
        }

        surface_cache_direct_lighting[int2(pixel_pos.xy)] = float4(point_lighting + directional_lighting, 1.0);
    }
}