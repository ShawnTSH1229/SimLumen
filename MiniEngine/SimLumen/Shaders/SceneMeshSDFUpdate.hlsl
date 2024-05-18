
#include "SimSDFCommon.hlsl"


RWTexture3D<float> global_distance_filed : register(u0);

// direction is outside the plane
static const float3 plane_direction[6] = {
    float3(+1,0,0), // direction_x_p
    float3(-1,0,0), // direction_x_n

    float3(0,+1,0), // direction_y_p
    float3(0,-1,0), // direction_y_n

    float3(0,0,+1), // direction_z_p
    float3(0,0,-1), // direction_z_n
};

[numthreads(LIGHT_PROBE_TEX_SIZE, LIGHT_PROBE_TEX_SIZE, 1)]
void GlobalSDFUpdateCS(uint3 dispatch_thread_idx : SV_DispatchThreadID)
{
    float3 global_sdf_voxel_offset = gloabl_sdf_voxel_size * dispatch_thread_idx;

    float3 voxel_start_position = gloabl_sdf_center - global_sdf_extents;
    float3 voxel_world_position = voxel_start_position + global_sdf_voxel_offset;

    //https://kosmonautblog.wordpress.com/2017/05/09/signed-distance-field-rendering-journey-pt-2/
    float min_plane_distance = 1e20f; 
    int min_sdf_distance_idx = -1;
    int min_dir_idx = -1;

    for(uint mesh_idx = 0; mesh_idx < scene_mesh_sdf_num; mesh_idx++)
    {
        SMeshSDFInfo mesh_sdf_info = scene_sdf_infos[mesh_idx];

        float3 voxel_volume_position = mul(mesh_sdf_info.world_to_volume,float4(voxel_world_position.xyz,1.0));

        for(uint dir_dix = 0; dir_idx < 6; dir_idx++)
        {
            float3 plane_center_position = mesh_sdf_info.volume_position_center + plane_direction[dir_idx] * mesh_sdf_info.volume_position_extent;
            float3 to_plane_distance = voxel_volume_position - plane_center_position;
            
            float distance = dot(to_plane_distance, plane_direction[dir_idx]);

            if(distance != 0.0f && distance < min_sdf_distance_idx)
            {
                min_plane_distance = distance;
                min_sdf_distance_idx = mesh_idx;
                min_dir_idx = dir_dix;
            }
        }
    }

    SMeshSDFInfo mesh_sdf_info = scene_sdf_infos[min_sdf_distance_idx];
    float3 sample_volume_position = mul(mesh_sdf_info.world_to_volume,float4(voxel_world_position.xyz,1.0)); 
    
    // out side volume
    if(distance > 0)
    {
        float3 line_volume_end_position = mesh_sdf_info.volume_position_center + mesh_sdf_info.volume_position_extent * plane_direction[min_dir_idx] * (-1.0 - 0.2);
        float2  intersect_t = LineBoxIntersect(
            sample_volume_position, line_volume_end_position,
            mesh_sdf_info.volume_position_center - mesh_sdf_info.volume_position_extent,
            mesh_sdf_info.volume_position_center + mesh_sdf_info.volume_position_extent);
        

        sample_volume_position = sample_volume_position + plane_direction[min_dir_idx] * (-1.0) * intersect_t.x;
    }

    // this is a approx distance
    float voxel_global_distance = SampleDistanceFieldBrickTexture(sample_volume_position,mesh_sdf_info);;

    if(distance > 0)
    {
        voxel_global_distance  = sqrt(voxel_global_distance * voxel_global_distance + min_plane_distance * min_plane_distance)
    }

    global_distance_filed[dispatch_thread_idx] = voxel_global_distance;
}
