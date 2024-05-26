#include "SimLumenCommon.hlsl"
#define VOXEL_UPDATE_GROUP_DATA 64
RWStructuredBuffer<SVoxelVisibilityInfo> scene_voxel_visibility_buffer : register(u0);

cbuffer SceneVoxelVisibilityInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

cbuffer CMeshSdfBrickTextureInfo : register(b1)
{
    // scene sdf infomation
    uint2 texture_brick_num_xy;
    float2 sdf_cb_padding0;

    uint3 texture_size_xyz;
    uint scene_mesh_sdf_num;
    
    // global sdf infomation
    float gloabl_sdf_voxel_size;
    float3 gloabl_sdf_center;

    float3 global_sdf_extents;
    float global_sdf_scale_x;

    float3 global_sdf_tex_size_xyz;
    float global_sdf_scale_y;
};

StructuredBuffer<SMeshSDFInfo> scene_sdf_infos : register(t0);
Texture3D<float> distance_field_brick_tex: register(t1);
SamplerState g_sampler_point_3d : register(s0);
#include "SimLumenSDFTraceCommon.hlsl"

// x: 48 * 48 * 160 / 64
// y: 6 direction
// 64

[numthreads(VOXEL_UPDATE_GROUP_DATA, 1, 1)]
void SceneVoxelVisibilityUpdate(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 thread_idx : SV_DispatchThreadID)
{
    uint voxel_index_1d = group_idx.x * VOXEL_UPDATE_GROUP_DATA + group_thread_idx.x;
    uint voxel_index_3d_z = voxel_index_1d / uint(SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y);

    uint voxel_slice_index = (voxel_index_1d % uint(SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y));
    uint voxel_index_3d_y = voxel_slice_index / SCENE_VOXEL_SIZE_X;
    uint voxel_index_3d_x = voxel_slice_index % SCENE_VOXEL_SIZE_X;

    uint3 vol_idx_3d = uint3(voxel_index_3d_x, voxel_index_3d_y, voxel_index_3d_z);

    float3 voxel_world_pos = vol_idx_3d * voxel_size + scene_voxel_min_pos + voxel_size * 0.5;

    uint direction_idx = group_idx.y;
    
    STraceResult trace_result = (STraceResult)0;
    trace_result.hit_distance = 1000.0f;

    [loop]
    for(uint mesh_idx = 0; mesh_idx < SCENE_SDF_NUM; mesh_idx++)
    {
        RayTraceSingleMeshSDF(voxel_world_pos, voxel_light_direction[direction_idx], 1000, mesh_idx, trace_result);
    }

    SVoxelDirVisInfo vis_info = (SVoxelDirVisInfo)0;
    vis_info.mesh_index = -1;
    if(trace_result.is_hit)
    {
        vis_info.mesh_index = trace_result.hit_mesh_index;
        vis_info.hit_distance = trace_result.hit_distance;
    }

    scene_voxel_visibility_buffer[voxel_index_1d].voxel_vis_info[direction_idx] = vis_info;
}