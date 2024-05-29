#include "SimLumenCommon.hlsl"
#define VOXEL_UPDATE_GROUP_DATA 64
cbuffer SceneVoxelVisibilityInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

StructuredBuffer<SVoxelVisibilityInfo> scene_voxel_visibility_buffer : register(t0);
StructuredBuffer<SMeshSDFInfo> scene_sdf_infos : register(t1);
StructuredBuffer<SCardInfo> scene_card_infos : register(t2);
Texture2D<float4>  final_lighting_tex          : register(t3);

RWStructuredBuffer<SVoxelLighting> scene_voxel_lighting: register(u0);

// x: 48 * 48 * 160 / 64
// y: 6 direction
// 64
[numthreads(VOXEL_UPDATE_GROUP_DATA, 1, 1)]
void LumenSceneLightInject(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 thread_idx : SV_DispatchThreadID)
{
    uint voxel_index_1d = group_idx.x * VOXEL_UPDATE_GROUP_DATA + group_thread_idx.x;
    uint voxel_index_3d_z = voxel_index_1d / uint(SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y);

    uint voxel_slice_index = (voxel_index_1d % uint(SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y));
    uint voxel_index_3d_y = voxel_slice_index / SCENE_VOXEL_SIZE_X;
    uint voxel_index_3d_x = voxel_slice_index % SCENE_VOXEL_SIZE_X;

    uint3 vol_idx_3d = uint3(voxel_index_3d_x, voxel_index_3d_y, voxel_index_3d_z);

    float3 voxel_world_pos = vol_idx_3d * voxel_size + scene_voxel_min_pos + voxel_size * 0.5;

    uint direction_idx = group_idx.y;
    SVoxelVisibilityInfo voxel_vis_info = scene_voxel_visibility_buffer[voxel_index_1d];
    int mesh_index = voxel_vis_info.voxel_vis_info[direction_idx].mesh_index;
    if(mesh_index != -1)
    {
        SMeshSDFInfo mesh_info = scene_sdf_infos[mesh_index];
        uint card_index = direction_idx; 
        uint global_card_index = mesh_info.mesh_card_start_index + card_index;

        SCardInfo card_info = scene_card_infos[global_card_index];

        float hit_distance = voxel_vis_info.voxel_vis_info[direction_idx].hit_distance;
        float3 light_direction = voxel_light_direction[direction_idx];
        float3 hit_world_pos = voxel_world_pos + light_direction * hit_distance;

        float2 uv = GetCardUVFromWorldPos(card_info, hit_world_pos);

        uint2 card_index_xy = uint2((global_card_index % card_num_xy), (global_card_index / card_num_xy));
        uint2 pixel_pos = card_index_xy * 128 + float2(uv * 128);
        pixel_pos.y = SURFACE_CACHE_TEX_SIZE - pixel_pos.y;
        float3 final_lighting = final_lighting_tex.Load(int3(pixel_pos,0)).xyz;

        scene_voxel_lighting[voxel_index_1d].final_lighting[direction_idx] = final_lighting;
    }

}