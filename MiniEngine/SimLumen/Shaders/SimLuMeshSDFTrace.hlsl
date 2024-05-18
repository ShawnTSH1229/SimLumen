#define LIGHT_PROBE_TEX_SIZE 8
#include "SimLuSceneCommon.hlsl"
#include "SimSDFCommon.hlsl"

//b0 s0 and t1 are used for scene mesh sdf

StructuredBuffer<SMeshSDFInfo> scene_sdf_infos: register(t0);
Buffer<uint> probe_num_per_trace_type: register(t0); //type 0, trace mesh sdf, type 1, trace voxel
Buffer<uint2> mesh_sdf_trace_probe_idx: register(t1);
Texture2D<float> probe_depth : register(t0);
StructuredBuffer<SSimLuMeshCard> scene_mesh_card_infos : register(t0);
StructuredBuffer<SSimLuCard> scene_card_infos : register(t0);
Texture2D<float3> surface_cache_final_light : register(t0);
Texture2D<uint> StructuredISIndirectTable: register(t0);

SamplerState gsampler_point_2d : register(s1);

RWTexture2D<float3> rw_trace_radiance:register(u0);
//todo: add a radiance clear pass

// probe num * 1 * 1
[numthreads(LIGHT_PROBE_TEX_SIZE, LIGHT_PROBE_TEX_SIZE, 1)]
void LightTraceMeshSDF(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID)
{
    uint mesh_sdf_prob_num = probe_num_per_trace_type[0];
    if(group_idx.x < mesh_sdf_prob_num)
    {
        uint2 probe_idx_xy = mesh_sdf_trace_probe_idx[group_idx.x];
        
        float probe_depth = probe_depth.Load(probe_idx_xy);
        
        uint2 pixel_idx_xy = probe_idx_xy * LIGHT_PROBE_TEX_SIZE + group_thread_idx.xy;
        float3 probe_world_pos = ;

        uint ray_info = StructuredISIndirectTable.Load(pixel_idx_xy);
        
        float2 probe_uv;
        GetProbeTracingUV(ray_info,probe_uv);
        float3 ray_direction = EquiAreaSphericalMapping(probe_uv);

        STraceResult trace_result;
        for(uint mesh_idx = 0; mesh_idx < scene_mesh_sdf_num; mesh_idx++)
        {
            RayTraceSingleMeshSDF(probe_world_pos,ray_direction,300,mesh_idx,trace_result);
        }

        if(trace_result.bHit)
        {
            SSimLuMeshCard mesh_card = scene_mesh_card_infos[trace_result.hit_mesh_card_idx];
            float3 sample_world_position = probe_world_pos + ray_direction * trace_result.hit_distance;
            float3 vector_to_center = sample_world_position - mesh_card.world_origin;
            
            vector_to_center = normalize(vector_to_center);

            float3 vector_to_center_abs = abs(vector_to_center);

            int max_dir = 0;
            float max_dist = 0;

            if(vector_to_center_abs.x > max_dist)
            {
                max_dist = vector_to_center_abs.x;
                max_dir = vector_to_center.x > 0 ? 0 : 1;
            }

            if(vector_to_center_abs.y > max_dist)
            {
                max_dist = vector_to_center_abs.y;
                max_dir = vector_to_center.y > 0 ? 2 : 3;
            }

            if(vector_to_center_abs.z > max_dist)
            {
                max_dist = vector_to_center_abs.z;
                max_dir = vector_to_center.z > 0 ? 4 : 4;
            }

            uint card_index = mesh_card.card_start_index + max_dir;
            SSimLuCard card_info = scene_card_infos[card_index];

            
            float3 mesh_card_space_position = mul(mesh_card.mesh_card_l2w_rotation,sample_world_position - mesh_card.world_origin);
            float3 card_space_position = mul(card_info.meshcard_to_card_space_rotation,mesh_card_space_position - card_info.card_space_position);
            
            float2 card_uv = clamp((card_space_position.xy + float2(0.5,0.5))/ card_info.card_extent_xy, 0.0, 0.9999f) * 128;
            uint2 surface_cache_index = card_info.atlas_card_index;
            uint2 surface_cache_pos = surface_cache_index * 128 + card_uv;

            float2 surface_cache_uv = surface_cache_pos / float2(4096.0f,4096.0f);

            float3 final_light = surface_cache_final_light.Sample(gsampler_point_2d,surface_cache_uv);
            rw_trace_radiance[pixel_idx_xy] = final_light;
        }
    }
}

