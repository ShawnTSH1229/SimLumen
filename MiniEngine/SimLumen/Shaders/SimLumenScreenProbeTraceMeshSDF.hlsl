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
StructuredBuffer<SMeshSDFInfo> scene_sdf_infos : register(t1);
Texture3D<float> distance_field_brick_tex: register(t2);
StructuredBuffer<SCardInfo> scene_card_infos : register(t0);
SamplerState g_sampler_point_3d : register(s0);
Texture2D<float4>  surface_cache_final_lighting          : register(t1);

RWTexture2D<uint> screen_space_probe_type : register(u0);
RWTexture2D<float3> screen_space_radiance : register(u1);

#include "SimLumenScreenSpaceProbeTraceCommon.hlsl"
#include "SimLumenSDFTraceCommon.hlsl"

[numthreads(PROBE_SIZE_2D,PROBE_SIZE_2D,1)]
void ScreenProbeTraceMeshSDFsCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 dispatch_thread_idx: SV_DispatchThreadID)
{
    uint2 ss_probe_idx_xy = group_idx.xy;
    uint2 ss_probe_atlas_pos = ss_probe_idx_xy * PROBE_SIZE_2D + PROBE_SIZE_2D / 2;
    float probe_depth = gbuffer_depth.Load(int3(ss_probe_atlas_pos.xy,0));
    
    float3 trace_radiance = float3(0,0,0);
    if(probe_depth != 0.0)
    {
       const float2 global_thread_size = float2(is_pdf_thread_size_x,is_pdf_thread_size_x);
       float2 piexl_tex_uv = float2(dispatch_thread_idx.xy) / global_thread_size;

       float3 probe_world_position = GetWorldPosByDepth(probe_depth, piexl_tex_uv);
       float probe_view_dist = length(probe_world_position - CameraPos);
       if(probe_view_dist < 100.0f)
       {
            float3 ray_direction;
            GetScreenProbeTexelRay(dispatch_thread_idx.xy, ray_direction);

            STraceResult trace_result = (STraceResult)0;
            trace_result.hit_distance = 1000.0f;

            [loop]
            for(uint mesh_idx = 0; mesh_idx < SCENE_SDF_NUM; mesh_idx++)
            {
                RayTraceSingleMeshSDF(probe_world_position, ray_direction, 1000, mesh_idx, trace_result);
            }

            float3 hit_world_position = float3(0.0,0.0,0.0);
            float3 hit_pos_normal = float3(0.0,0.0,0.0);
            if(trace_result.is_hit)
            {
                hit_world_position = probe_world_position + ray_direction * trace_result.hit_distance;

                float max_ray_dir = 0.0;
                int max_dir_card_idx = 0;

                if(abs(ray_direction.x) > max_ray_dir)
                {
                    if(ray_direction.x > 0){max_dir_card_idx = 5;}
                    else{max_dir_card_idx = 4;};
                }

                if(abs(ray_direction.y) > max_ray_dir)
                {
                    if(ray_direction.y > 0){max_dir_card_idx = 3;}
                    else{max_dir_card_idx = 2;};
                }
                
                if(abs(ray_direction.z) > max_ray_dir)
                {
                    if(ray_direction.z > 0){max_dir_card_idx = 1;}
                    else{max_dir_card_idx = 0;};
                }

                SMeshSDFInfo mesh_sdf_info = scene_sdf_infos[trace_result.hit_mesh_index];
                uint card_index = mesh_sdf_info.mesh_card_start_index + max_dir_card_idx;
                SCardInfo card_info = scene_card_infos[card_index];

                float2 card_uv = GetCardUVFromWorldPos(card_info, hit_world_position);
                uint2 card_index_xy = uint2((card_index % card_num_xy), (card_index / card_num_xy));
                uint2 pixel_pos = card_index_xy * 128 + float2(card_uv * 128);
                pixel_pos.y = SURFACE_CACHE_TEX_SIZE - pixel_pos.y;
                float3 lighting = surface_cache_final_lighting.Load(int3(pixel_pos,0)).xyz;
                trace_radiance = lighting;
            }
            else
            {
                trace_radiance = float3(0.1,0.1,0.1);
            }
            screen_space_probe_type[dispatch_thread_idx.xy] = 1;
       }
    }
    else
    {
        screen_space_probe_type[dispatch_thread_idx.xy] = 0;
    }
    screen_space_radiance[dispatch_thread_idx.xy] = trace_radiance;

}