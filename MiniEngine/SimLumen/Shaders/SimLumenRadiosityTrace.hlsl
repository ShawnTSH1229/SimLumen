#include "SimLumenCommon.hlsl"

cbuffer CBLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

cbuffer CMeshSdfBrickTextureInfo : register(b1)
{
    GLOBAL_SDF_BUFFER_MEMBER
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

//2048 / 16, 512 / 16 
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
        TraceGlobalSDF(card_data.world_position + card_data.world_normal * gloabl_sdf_voxel_size * 2.0, world_ray, hit_result);

        int x_dir = 0;
        if(world_ray.x > 0.0) { x_dir = 5;  }
        else { x_dir = 4;  }

        int y_dir = 0;
        if(world_ray.y > 0.0) { y_dir = 2;  }
        else { y_dir = 2;  }

        int z_dir = 0;
        if(world_ray.z > 0.0) { z_dir = 1;  }
        else { z_dir = 0;  }

        int max_dir_idx = -1;
        float max_dir = 0.0;
        
         if(hit_result.bHit)
         {
            float3 hit_world_position = 
                 world_ray * (hit_result.hit_distance - gloabl_sdf_voxel_size) + 
                 card_data.world_position + 
                 card_data.world_normal * gloabl_sdf_voxel_size * 2.0;

            uint voxel_index_1d = GetVoxelIndexFromWorldPos(hit_world_position);

            SVoxelLighting voxel_lighting = scene_voxel_lighting[voxel_index_1d];
            float3 voxel_lighting_x = voxel_lighting.final_lighting[x_dir];
            float3 voxel_lighting_y = voxel_lighting.final_lighting[y_dir];
            float3 voxel_lighting_z = voxel_lighting.final_lighting[z_dir];

            float weight_x = saturate(dot(world_ray, voxel_light_direction[x_dir]));
            float weight_y = saturate(dot(world_ray, voxel_light_direction[y_dir]));
            float weight_z = saturate(dot(world_ray, voxel_light_direction[z_dir]));

            radiance += voxel_lighting_x * weight_x;
            radiance += voxel_lighting_y * weight_y;
            radiance += voxel_lighting_z * weight_z;

            radiance /= (weight_x + weight_y + weight_z);
            radiance = radiance * (1.0 / PI);

            float max_lighting = max(radiance.x, max(radiance.y, radiance.z));
            if(max_lighting > 1* (1.0 / PI))
            {
                 radiance *= (1* (1.0 / PI) / max_lighting);
            }
         }
         else
         {
             radiance = float3(0.01, 0.01, 0.01); // hack sky light
         }
    } 
    trace_radiance_atlas[pixel_atlas_pos.xy] = float4(radiance,0.0);
}