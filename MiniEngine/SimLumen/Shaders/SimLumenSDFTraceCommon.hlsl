#ifndef SIMLUMEN_SDF_TRACE_COMMON
#define SIMLUMEN_SDF_TRACE_COMMON

#include "SimLumenCommon.hlsl"

float SampleDistanceFieldBrickTexture(float3 sample_position, SMeshSDFInfo mesh_sdf_info)
{
    int brick_start_idx = mesh_sdf_info.volume_brick_start_idx;
    float volume_brick_szie = mesh_sdf_info.volume_brick_size;

    float3 volume_min_pos = mesh_sdf_info.volume_position_center - mesh_sdf_info.volume_position_extent;
    float3 sample_position_offset = sample_position - volume_min_pos;

    float voxel_size = volume_brick_szie * 0.125;
    int3 brick_index_xyz =  sample_position_offset / volume_brick_szie;
    int3 in_brick_index_xyz = (sample_position_offset - brick_index_xyz * volume_brick_szie) / voxel_size;

    int global_brick_index = 
        brick_start_idx + 
        brick_index_xyz.x + 
        brick_index_xyz.y * mesh_sdf_info.volume_brick_num_xyz.x + 
        brick_index_xyz.z * mesh_sdf_info.volume_brick_num_xyz.x * mesh_sdf_info.volume_brick_num_xyz.y;
    
    int global_brick_index_x =  global_brick_index % texture_brick_num_xy.x;
    int global_brick_index_y =  global_brick_index / texture_brick_num_xy.x;

    int3 global_texel_index = int3(global_brick_index_x, global_brick_index_y, 0) * 8.0 + in_brick_index_xyz;
    float3 global_texel_uvw = float3(global_texel_index) / float3(texture_size_xyz);

    float distance = distance_field_brick_tex.Sample(g_sampler_point_3d, global_texel_uvw);
    return distance * mesh_sdf_info.sdf_distance_scale.x + mesh_sdf_info.sdf_distance_scale.y;
};

//The intersections will always be in the range [0,1], which corresponds to [RayOrigin, RayEnd] in worldspace.
float2 LineBoxIntersect(float3 ray_origin, float3 ray_end, float3 box_min, float3 box_max)
{
    float3 inv_ray_dir = 1.0f / (ray_end - ray_origin);

    float3 first_plane_intersections = (box_min - ray_origin) * inv_ray_dir;
    float3 second_plane_intersections = (box_max - ray_origin) * inv_ray_dir;

    float3 closest_plane_intersections = min(first_plane_intersections, second_plane_intersections);
    float3 furthest_plane_intersections = max(first_plane_intersections, second_plane_intersections);

    float2 box_intersections;
    box_intersections.x = max(closest_plane_intersections.x, max(closest_plane_intersections.y, closest_plane_intersections.z));
    box_intersections.y = min(furthest_plane_intersections.x, min(furthest_plane_intersections.y, furthest_plane_intersections.z));
    return saturate(box_intersections);
};

void RayTraceSingleMeshSDF(float3 world_ray_start,float3 world_ray_direction,float max_trace_distance, uint object_index, in out STraceResult trace_result)
{
    SMeshSDFInfo mesh_sdf_info = scene_sdf_infos[object_index];
    
    float3 world_ray_end = world_ray_start + world_ray_direction * max_trace_distance;
    float3 volume_ray_start = mul(mesh_sdf_info.world_to_volume, float4(world_ray_start, 1.0)).xyz;
    float3 volume_ray_end = mul(mesh_sdf_info.world_to_volume, float4(world_ray_end, 1.0)).xyz;

    float3 volume_min_pos = mesh_sdf_info.volume_position_center - mesh_sdf_info.volume_position_extent;
    float3 volume_max_pos = mesh_sdf_info.volume_position_center + mesh_sdf_info.volume_position_extent;

    float2 volume_space_intersection_times = LineBoxIntersect(volume_ray_start, volume_ray_end, volume_min_pos, volume_max_pos);

    float3 volume_ray_direction = volume_ray_end - volume_ray_start;
    float volume_max_trace_distance = length(volume_ray_direction);
    volume_ray_direction /= volume_max_trace_distance;
    volume_space_intersection_times *= volume_max_trace_distance;

    //If the ray did not intersect the box, then the furthest intersection <= the closest intersection.
    if((volume_space_intersection_times.x < volume_space_intersection_times.y) && (volume_space_intersection_times.x < trace_result.hit_distance))
    {
        float sample_ray_t = volume_space_intersection_times.x;

        uint max_step = 64;
        bool bhit = false;
        uint step_idx = 0;

        [loop]
        for( ; step_idx < max_step; step_idx++)
        {
            float3 sample_volume_position = volume_ray_start + volume_ray_direction * sample_ray_t;
            float distance_filed = SampleDistanceFieldBrickTexture(sample_volume_position, mesh_sdf_info);
            float min_hit_distance = mesh_sdf_info.volume_brick_size * 0.125 * 1.0; // 1 voxel

            if(distance_filed < min_hit_distance)
            {
                bhit = true;
                sample_ray_t = clamp(sample_ray_t + distance_filed - min_hit_distance, volume_space_intersection_times.x, volume_space_intersection_times.y);
                break;
            }

            sample_ray_t += distance_filed;

            if(sample_ray_t > volume_space_intersection_times.y + min_hit_distance)
            {
                break;
            }
        }

        if(step_idx == max_step)
        {
            bhit = true;
        }

        if(bhit && sample_ray_t < trace_result.hit_distance)
        {
            trace_result.is_hit = true;   
            trace_result.hit_distance = sample_ray_t;
            trace_result.hit_mesh_index = object_index;
            trace_result.hit_mesh_sdf_card_index = mesh_sdf_info.mesh_card_start_index;
        }
    }
};

float3 CalculateMeshSDFGradient(float3 sample_volume_position, SMeshSDFInfo mesh_sdf_info)
{
    float voxel_offset = mesh_sdf_info.volume_brick_size * 0.125;

    float R = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x + voxel_offset, sample_volume_position.y, sample_volume_position.z),mesh_sdf_info);
    float L = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x - voxel_offset, sample_volume_position.y, sample_volume_position.z),mesh_sdf_info);

    float F = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x, sample_volume_position.y + voxel_offset, sample_volume_position.z),mesh_sdf_info);
    float B = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x, sample_volume_position.y - voxel_offset, sample_volume_position.z),mesh_sdf_info);

    float U = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x, sample_volume_position.y, sample_volume_position.z + voxel_offset),mesh_sdf_info);
    float D = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x, sample_volume_position.y, sample_volume_position.z - voxel_offset),mesh_sdf_info);

    float3 gradiance = float3(R - L, F - B, U - D);
	return gradiance;
}

float3 CalculateMeshSDFWorldNormal(float3 world_ray_start, float3 world_ray_direction, STraceResult hit_result)
{
    SMeshSDFInfo mesh_sdf_info = scene_sdf_infos[hit_result.hit_mesh_index];
    float3 hit_world_position = world_ray_start + world_ray_direction * hit_result.hit_distance;
    float3 hit_volume_position = mul(mesh_sdf_info.world_to_volume, float4(hit_world_position.xyz,1.0));
    hit_volume_position = clamp(hit_volume_position, mesh_sdf_info.volume_position_center - mesh_sdf_info.volume_position_extent, mesh_sdf_info.volume_position_center + mesh_sdf_info.volume_position_extent);

    // volume normal
    float3 volume_gradient = CalculateMeshSDFGradient(hit_volume_position, mesh_sdf_info);
    float gradient_length = length(volume_gradient);
    float3 volume_normal = gradient_length > 0.00001f ? volume_gradient / gradient_length : 0;
    
    // world normal
    float3 world_gradient = mul((float3x3)mesh_sdf_info.volume_to_world,volume_normal);
    float world_gradient_length = length(world_gradient);
    float3 world_normal = world_gradient_length > 0.00001f ? world_gradient / world_gradient_length : 0;
    
    return world_normal;
}
#endif