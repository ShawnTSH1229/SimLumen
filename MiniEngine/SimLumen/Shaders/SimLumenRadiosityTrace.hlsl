#include "SimLumenCommon.hlsl"

#define MAX_FRAME_ACCUMULATED 4

cbuffer CBLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

StructuredBuffer<SCardInfo> scene_card_infos : register(t0);
Texture2D<float4> scene_card_albedo : register(t1);
Texture2D<float4> scene_card_normal : register(t2);
Texture2D<float> scene_card_depth : register(t3);
Texture3D<float> global_sdf_texture: register(t4);
StructuredBuffer<SVoxelLighting> scene_voxel_lighting: register(t5);

SamplerState g_global_sdf_sampler : register(s0);

RWTexture2D<float4> trace_radiance_atlas : register(u0);

#include "SimLumenRadiosityCommon.hlsl"
#include "SimLumenSurfaceCacheCommon.hlsl"
#include "SimLumenGlobalSDFTraceCommon.hlsl"

// 512 * 2048 / 256
#define RADIOSITY_TRACE_THREADGROUP_SIZE 16
[numthreads(RADIOSITY_TRACE_THREADGROUP_SIZE, RADIOSITY_TRACE_THREADGROUP_SIZE, 1)]
void LumenRadiosityDistanceFieldTracingCS(uint3 thread_index : SV_DispatchThreadID)
{
    uint2 card_idx_2d = thread_index.xy / SURFACE_CACHE_CARD_SIZE;
    uint2 sub_card_idx = thread_index.xy % SURFACE_CACHE_CARD_SIZE;
    uint2 tile_idx = sub_card_idx / SURFACE_CACHE_PROBE_TEXELS_SIZE;
    uint card_index_1d = card_idx_2d.y * SURFACE_CACHE_CARD_NUM_XY + card_idx_2d.x;

    uint indirect_lighting_temporal_index = tile_idx.y * (SURFACE_CACHE_CARD_SIZE / 4) + tile_idx.x + frame_index;
    uint2 probe_jitter = GetProbeJitter(indirect_lighting_temporal_index);
    
    uint2 probe_start_pos = card_idx_2d * SURFACE_CACHE_CARD_SIZE + tile_idx * SURFACE_CACHE_PROBE_TEXELS_SIZE;
    uint2 probe_center_pos = probe_start_pos + probe_jitter;
    uint2 sub_tile_pos = thread_index.xy - probe_start_pos;

    uint2 probe_atlas_pos = probe_center_pos;
    probe_atlas_pos.y = SURFACE_CACHE_TEX_SIZE - probe_center_pos.y;

    uint2 pixel_atlas_pos = thread_index.xy;
    pixel_atlas_pos.y = SURFACE_CACHE_TEX_SIZE - pixel_atlas_pos.y;

    float depth = scene_card_depth.Load(int3(probe_atlas_pos.xy,0));
    float3 radiance = float3(0,0,0);

    if(depth != 0)
    {
        SCardInfo card_info = scene_card_infos[card_index_1d];
        SCardData card_data = GetSurfaceCardData(card_info, float2(uint2(thread_index.xy % 128u)) / 128.0f, pixel_atlas_pos.xy);

        float3 world_ray;
        float pdf;
        GetRadiosityRay(tile_idx, sub_tile_pos, card_data.world_normal, world_ray, pdf);

        SGloablSDFHitResult hit_result = (SGloablSDFHitResult)0;
        TraceGlobalSDF(card_data.world_position, world_ray, hit_result);

        int max_dir_idx = -1;
        float max_dir = 0.0;

        if(abs(world_ray.x) > max_dir)
        {
            if(world_ray.x > 0.0) { max_dir_idx = 5;  }
            else { max_dir_idx = 4;  }

            max_dir = abs(world_ray.x);
        }

        if(abs(world_ray.y) > max_dir)
        {
            if(world_ray.y > 0.0) { max_dir_idx = 3;  }
            else { max_dir_idx = 2;  }

            max_dir = abs(world_ray.y);
        }

        if(abs(world_ray.z) > max_dir)
        {
            if(world_ray.z > 0.0) { max_dir_idx = 1;  }
            else { max_dir_idx = 0;  }

            max_dir = abs(world_ray.z);
        }
        
        if(max_dir_idx > -1)
        {
            if(hit_result.bHit)
            {
                float3 hit_world_position = world_ray * hit_result.hit_distance + card_data.world_position;
                uint voxel_index_1d = GetVoxelIndexFromWorldPos(hit_world_position);
                SVoxelLighting voxel_lighting = scene_voxel_lighting[voxel_index_1d];
                radiance = voxel_lighting.final_lighting;
            }
            else
            {
                radiance = float3(0.01, 0.01, 0.01); // hack sky light
            }
            
        }
    } 
    trace_radiance_atlas[pixel_atlas_pos.xy] = float4(radiance,0.0);
}