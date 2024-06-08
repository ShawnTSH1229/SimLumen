#include "SimLumenCommon.hlsl"
#define PROBE_SIZE_2D 8

cbuffer CLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

cbuffer CLumenViewInfo : register(b1)
{
    GLOBAL_VIEW_CONSTANT_BUFFER
};

cbuffer CMeshSdfBrickTextureInfo : register(b2)
{
    GLOBAL_SDF_BUFFER_MEMBER
};

Texture2D<float> gbuffer_depth                          : register(t0);
Texture2D<uint> structed_is_indirect_table              : register(t1);
StructuredBuffer<SVoxelLighting> scene_voxel_lighting   : register(t2);
Texture3D<float> global_sdf_texture                     : register(t3);
Texture2D<float4> gbuffer_b                             : register(t4);
Texture2D<float4> gbuffer_c                             : register(t5);//todo: fix me

RWTexture2D<float3> screen_space_radiance : register(u0);

SamplerState g_global_sdf_sampler : register(s0);

#include "SimLumenGlobalSDFTraceCommon.hlsl"
#include "SimLumenScreenSpaceProbeTraceCommon.hlsl"
#include "SimLumenRadiosityCommon.hlsl"

[numthreads(PROBE_SIZE_2D,PROBE_SIZE_2D,1)]
void ScreenProbeTraceVoxelCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 dispatch_thread_idx: SV_DispatchThreadID)
{
    uint2 ss_probe_idx_xy = group_idx.xy;
    uint2 ss_probe_atlas_pos = ss_probe_idx_xy * PROBE_SIZE_2D + PROBE_SIZE_2D / 2;
    float probe_depth = gbuffer_depth.Load(int3(ss_probe_atlas_pos.xy,0));
    float3 probe_world_position = gbuffer_c.Load(int3(dispatch_thread_idx.xy,0)).xyz;//todo: fix me

    float3 radiance = float3(0,0,0);
    if(probe_depth != 0.0)
    {
       float probe_view_dist = length(probe_world_position - CameraPos);
       if(probe_view_dist >= 1000.0f)
       {    
            float3 ray_direction;
            GetScreenProbeTexelRay(dispatch_thread_idx.xy, ray_direction);

            float3 world_normal = gbuffer_b.Load(int3(ss_probe_atlas_pos.xy,0)).xyz * 2.0 - 1.0;

            SGloablSDFHitResult hit_result = (SGloablSDFHitResult)0;
            TraceGlobalSDF(probe_world_position + world_normal * gloabl_sdf_voxel_size * 2.0, ray_direction, hit_result);

            int max_dir_idx = -1;
            float max_dir = 0.0;

            int x_dir = 0;
            if(abs(ray_direction.x) > max_dir)
            {
                if(ray_direction.x > 0.0) { x_dir = 5;  }
                else { x_dir = 4;  }
                max_dir_idx = x_dir;
            }


            int y_dir = 0;
            if(abs(ray_direction.y) > max_dir)
            {
                if(ray_direction.y > 0.0) { y_dir = 3;  }
                else { y_dir = 2;  }
                max_dir_idx = y_dir;
            }


            int z_dir = 0;
            if(abs(ray_direction.z) > max_dir)
            {
                if(ray_direction.z > 0.0) { z_dir = 1;  }
                else { z_dir = 0;  }
                max_dir_idx = z_dir;
            }


            if(hit_result.bHit)
            {
                float3 hit_world_position = 
                 ray_direction * (hit_result.hit_distance - gloabl_sdf_voxel_size) + 
                 probe_world_position  + 
                 world_normal * gloabl_sdf_voxel_size * 2.0;

                uint voxel_index_1d = GetVoxelIndexFromWorldPos(hit_world_position);

                SVoxelLighting voxel_lighting = scene_voxel_lighting[voxel_index_1d];
            #if 0
                float3 voxel_lighting_x = voxel_lighting.final_lighting[x_dir];
                float3 voxel_lighting_y = voxel_lighting.final_lighting[y_dir];
                float3 voxel_lighting_z = voxel_lighting.final_lighting[z_dir];

                float weight_x = saturate(dot(ray_direction, voxel_light_direction[x_dir]));
                float weight_y = saturate(dot(ray_direction, voxel_light_direction[y_dir]));
                float weight_z = saturate(dot(ray_direction, voxel_light_direction[z_dir]));

                radiance += (voxel_lighting_x * weight_x);
                radiance += (voxel_lighting_y * weight_y);
                radiance += (voxel_lighting_z * weight_z);

                radiance /= (weight_x + weight_y + weight_z);
            #else
                radiance = voxel_lighting.final_lighting[max_dir_idx] * 1.2/*todo: fixme*/;
            #endif
            }
            else
            {
                radiance = float3(0.1,0.1,0.1); //hack sky light
            }

            screen_space_radiance[dispatch_thread_idx.xy] = float4(radiance,1.0);
       }
    }
}