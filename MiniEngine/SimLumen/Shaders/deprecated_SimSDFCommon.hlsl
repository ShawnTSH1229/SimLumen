#include "SimLuSceneCommon.hlsl"

cbuffer CMeshSdfBrickTextureInfo : register(b0)
{
    // scene sdf infomation
    uint2 texture_brick_num_xy;
    uint3 texture_size_xyz;
    uint scene_mesh_sdf_num;
    
    // global sdf infomation
    float gloabl_sdf_voxel_size;
    float3 gloabl_sdf_center;
    float3 global_sdf_extents;
};

SamplerState g_sampler_point_3d : register(s0);
StructuredBuffer<SMeshSDFInfo>scene_sdf_infos:register(t0);

Texture3D<float> distance_field_brick_tex: register(t0);

//The intersections will always be in the range [0,1], which corresponds to [RayOrigin, RayEnd] in worldspace.
float2 LineBoxIntersect(float3 ray_origin, float3 ray_end, float3 box_min, float3 box_max)
{
    float3 inv_ray_dir = 1.0f / (ray_end - ray_origin);

    float3 first_plane_intersections = (box_min - ray_origin) * inv_ray_dir;
    float3 second_plane_intersections = (box_max - ray_origin) * inv_ray_dir;

    float3 closest_plane_intersections = min(first_plane_intersections,second_plane_intersections);
    float3 furthest_plane_intersections = max(first_plane_intersections,second_plane_intersections);

    float2 box_intersections;
    box_intersections.x = max(closest_plane_intersections.x,max(closest_plane_intersections.y,closest_plane_intersections.z));
    box_intersections.y = min(furthest_plane_intersections.x,min(furthest_plane_intersections.y,furthest_plane_intersections.z));
    return box_intersections;
}


//todo: check voxel center position, we set to 0.5 0.5 in the voxel
float SampleDistanceFieldBrickTexture(float3 sample_position, SMeshSDFInfo mesh_sdf_info)
{
    int brick_start_idx = mesh_sdf_info.volume_brick_start_idx;
    float volume_brick_szie = mesh_sdf_info.volume_brick_size;

    //must be int
    float3 volume_min_pos = mesh_sdf_info.volume_position_center - mesh_sdf_info.volume_position_extent;
    float3 sample_position_offset = sample_position - volume_min_pos;

    int3 brick_index_xyz =  sample_position_offset / volume_brick_szie;//e.g. 0-8 -> 0 - 1 -> 0
    int3 in_brick_index_xyz = sample_position_offset - brick_index_xyz * volume_brick_szie; // 0 - 5

    int global_brick_index = 
        brick_start_idx + 
        brick_index_xyz.x + 
        brick_index_xyz.y * volume_brick_num_xyz.x + 
        brick_index_xyz.z * volume_brick_num_xyz.x * volume_brick_num_xyz.y;

    int global_brick_index_x =  global_brick_index % texture_brick_num_xy.x;
    int global_brick_index_y =  global_brick_index / texture_brick_num_xy.x;

    int global_texel_index = int3(global_brick_index_x,global_brick_index_y,0) + in_brick_index_xyz;
    float3 global_texel_uvw = global_texel_index / texture_size_xyz;
    
    float distance = distance_field_brick_tex.Sample(g_sampler_point_3d,global_texel_uvw);
    return distance * mesh_sdf_info.sdf_distance_scale.x + sdf_distance_scale.y;
}

float3 CalculateMeshSDFGradient(float3 sample_volume_position, SMeshSDFInfo mesh_sdf_info)
{
    float voxel_offset = mesh_sdf_info.volume_brick_size * 0.125;

    float R = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x + voxel_offset, sample_volume_position.y, sample_volume_position.z),mesh_sdf_info);
    float L = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x - voxel_offset, sample_volume_position.y, sample_volume_position.z),mesh_sdf_info);

    float F = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x, sample_volume_position.y + voxel_offset, sample_volume_position.z),mesh_sdf_info);
    float B = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x, sample_volume_position.y + voxel_offset, sample_volume_position.z),mesh_sdf_info);

    float U = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x, sample_volume_position.y, sample_volume_position.z + voxel_offset),mesh_sdf_info);
    float D = SampleDistanceFieldBrickTexture(float3(sample_volume_position.x, sample_volume_position.y, sample_volume_position.z + voxel_offset),mesh_sdf_info);

    float3 gradiance = float3(R - L, F - B, U - D);
	return gradiance;
}


float3 CalculateMeshSDFWorldNormal(float3 world_ray_start, float3 world_ray_direction, SDFTraceResult hit_result)
{
    SMeshSDFInfo mesh_sdf_info = scene_sdf_infos[hit_result.hit_mesh_index];
    float3 hit_world_position = world_ray_start + world_ray_direction * hit_result.hit_distance;
    
    float3 hit_volume_position = mul(mesh_sdf_info.world_to_volume,float4(hit_world_position.xyz,1.0));
    hit_volume_position = clamp(hit_volume_position, mesh_sdf_info.volume_position_center - volume_position_extent,mesh_sdf_info.volume_position_center + volume_position_extent);

    // volume normal
    float3 volume_gradient = CalculateMeshSDFGradient(hit_volume_position, mesh_sdf_info);
    float gradient_length = length(volume_gradient);
    float3 volume_normal = gradient_length > 0.00001f ? volume_gradient / gradient_length : 0;
    
    // world normal
    float3 world_gradient = mul(transpose((float3x3)mesh_sdf_info.world_to_volume),volume_normal);
    float world_gradient_length = length(world_gradient);
    float3 world_normal = world_gradient_length > 0.00001f ? world_gradient / world_gradient_length : 0;
    
    return world_normal;
}

void RayTraceSingleMeshSDF(float3 world_ray_start,float3 world_ray_direction,float max_trace_distance, uint object_index, in out STraceResult SDFTraceResult)
{
    SMeshSDFInfo mesh_sdf_info = scene_sdf_infos[object_index];
    
    float3 world_ray_end = world_ray_start + world_ray_direction * max_trace_distance;
    float3 volume_ray_start = mul(mesh_sdf_info.world_to_volume,float4(world_ray_start,1.0)); //todo: is this valid?
    float3 volume_ray_end = mul(mesh_sdf_info.world_to_volume,float4(world_ray_end,1.0)); //todo: is this valid?

    float3 volume_min_pos = mesh_sdf_info.volume_position_center - mesh_sdf_info.volume_position_extent;
    float3 volume_max_pos = mesh_sdf_info.volume_position_center - mesh_sdf_info.volume_position_extent;

    float2 volume_space_intersection_times = LineBoxIntersect(volume_ray_start, volume_ray_end, volume_min_pos, volume_max_pos);
    
    float3 volume_ray_direction = volume_ray_end - volume_ray_start;
    float volume_max_trace_distance = length(volume_ray_direction);
    //float volume_min_trace_distance = volume_max_trace_distance * min_trace_distance / max_trace_distance;

    // normalize
    volume_ray_direction /= volume_max_trace_distance;

    SDFTraceResult trace_result;
    trace_result.hit_distance = false;

    //If the ray did not intersect the box, then the furthest intersection <= the closest intersection.
    if(volume_space_intersection_times.x < volume_space_intersection_times.y)
    {
        float sample_ray_t = volume_space_intersection_times.x;
        
        uint max_step = 64;
        bool bhit = false;
        uint step_idx = 0;
        for( ; step_idx < max_step; step_idx++)
        {
            float3 sample_volume_position = volume_ray_start + volume_ray_direction * sample_ray_t;

            float distance_filed = SampleDistanceFieldBrickTexture(sample_volume_position,mesh_sdf_info);
            float min_hit_distance = mesh_sdf_info.volume_brick_size * 0.125 * 2.0; // 2 voxel , use 1.5 volxel ?

            if(distance_filed < min_hit_distance)
            {
                bhit = true;

                //todo: fix me
                sample_ray_t = clamp(sample_ray_t + distance_filed - min_hit_distance,volume_space_intersection_times.x,volume_space_intersection_times.y);
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
            trace_result.bHit = true;   
            trace_result.hit_distance = sample_ray_t;
            trace_result.hit_mesh_index = object_index;
            trace_result.hit_mesh_card_idx = mesh_sdf_info.mesh_card_index;
        }
    }
    
}
