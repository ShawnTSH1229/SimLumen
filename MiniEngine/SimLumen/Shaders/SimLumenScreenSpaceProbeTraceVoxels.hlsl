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

Texture2D<float> gbuffer_depth      : register(t0);
Texture2D<uint> structed_is_indirect_table: register(t1);
StructuredBuffer<SVoxelLighting> scene_voxel_lighting: register(t5);

RWTexture2D<uint> screen_space_probe_type : register(u0);
RWTexture2D<float3> screen_space_radiance : register(u1);
#include "SimLumenGlobalSDFTraceCommon.hlsl"
#include "SimLumenScreenSpaceProbeTraceCommon.hlsl"
#include "SimLumenRadiosityCommon.hlsl"

[numthreads(PROBE_SIZE_2D,PROBE_SIZE_2D,1)]
void ScreenProbeTraceMeshSDFsCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 dispatch_thread_idx: SV_DispatchThreadID)
{
    uint2 ss_probe_idx_xy = group_idx.xy;
    uint2 ss_probe_atlas_pos = ss_probe_idx_xy * PROBE_SIZE_2D + PROBE_SIZE_2D / 2;
    float probe_depth = gbuffer_depth.Load(int3(ss_probe_atlas_pos.xy,0));

    float3 radiance = float3(0,0,0);
    if(probe_depth != 0.0)
    {
       const float2 global_thread_size = float2(is_pdf_thread_size_x,is_pdf_thread_size_x);
       float2 piexl_tex_uv = float2(dispatch_thread_idx.xy) / global_thread_size;

       float3 probe_world_position = GetWorldPosByDepth(probe_depth, piexl_tex_uv);
       float probe_view_dist = length(probe_world_position - CameraPos);
       if(probe_view_dist > 100.0f)
       {    
            float3 ray_direction;
            GetScreenProbeTexelRay(dispatch_thread_idx.xy, ray_direction);

            SGloablSDFHitResult hit_result = (SGloablSDFHitResult)0;
            TraceGlobalSDF(probe_world_position + ray_direction * gloabl_sdf_voxel_size * 2.0, ray_direction, hit_result);

            int x_dir = 0;
            if(ray_direction.x > 0.0) { x_dir = 5;  }
            else { x_dir = 4;  }

            int y_dir = 0;
            if(ray_direction.y > 0.0) { y_dir = 2;  }
            else { y_dir = 2;  }

            int z_dir = 0;
            if(ray_direction.z > 0.0) { z_dir = 1;  }
            else { z_dir = 0;  }

            int max_dir_idx = -1;
            float max_dir = 0.0;

            if(hit_result.bHit)
            {
                float3 hit_world_position = 
                 ray_direction * (hit_result.hit_distance - gloabl_sdf_voxel_size) + 
                 probe_world_position  + 
                 ray_direction * gloabl_sdf_voxel_size * 2.0;

                uint voxel_index_1d = GetVoxelIndexFromWorldPos(hit_world_position);

                SVoxelLighting voxel_lighting = scene_voxel_lighting[voxel_index_1d];
                float3 voxel_lighting_x = voxel_lighting.final_lighting[x_dir];
                float3 voxel_lighting_y = voxel_lighting.final_lighting[y_dir];
                float3 voxel_lighting_z = voxel_lighting.final_lighting[z_dir];

                float weight_x = saturate(dot(ray_direction, voxel_light_direction[x_dir]));
                float weight_y = saturate(dot(ray_direction, voxel_light_direction[y_dir]));
                float weight_z = saturate(dot(ray_direction, voxel_light_direction[z_dir]));

                radiance += voxel_lighting_x * weight_x;
                radiance += voxel_lighting_y * weight_y;
                radiance += voxel_lighting_z * weight_z;

                radiance /= (weight_x + weight_y + weight_z);

                screen_space_radiance[dispatch_thread_idx.xy] = radiance;
            }
            screen_space_probe_type[dispatch_thread_idx.xy] = 2;
       }
    }
}