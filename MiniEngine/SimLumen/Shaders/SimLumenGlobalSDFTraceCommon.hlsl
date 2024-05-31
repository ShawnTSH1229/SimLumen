#ifndef SIMLUMEN_GLOBAL_SDF_TRACE_COMMON
#define SIMLUMEN_GLOBAL_SDF_TRACE_COMMON

#include "SimLumenCommon.hlsl"

float SampleGlobalSDF(float3 sample_position_world_space)
{
    const float3 global_sdf_min_pos = gloabl_sdf_center - global_sdf_extents;

    float3 sample_sdf_offset = sample_position_world_space - global_sdf_min_pos;
    float3 sample_sdf_index_xyz = sample_sdf_offset / gloabl_sdf_voxel_size;
    float3 sample_sdf_uvw = sample_sdf_index_xyz / global_sdf_tex_size_xyz;
    float distance = global_sdf_texture.SampleLevel(g_global_sdf_sampler, sample_sdf_uvw,0); // todo: fix me 
    return distance * global_sdf_scale_x + global_sdf_scale_y;
}

void TraceGlobalSDF(float3 world_ray_start, float3 world_ray_direction, inout SGloablSDFHitResult hit_result)
{
    hit_result.bHit = false;
    float3 global_sdf_center_distance = abs(world_ray_start - gloabl_sdf_center);
    if(all(global_sdf_center_distance < global_sdf_extents))
    {
        float sample_ray_t = gloabl_sdf_voxel_size * 1.0f;

        float3 norm_world_dir = normalize(world_ray_direction);

        uint max_step = 64;
        bool bhit = false;
        uint step_idx = 0;
        [loop]
        for( ; step_idx < max_step; step_idx++)
        {
            float3 sample_world_position = world_ray_start + norm_world_dir * sample_ray_t;
            float3 sample_sdf_center_distance = abs(sample_world_position - gloabl_sdf_center);
            if(any(bool3(sample_sdf_center_distance > global_sdf_extents)))
            {
                bhit = false;
                break;
            }

            float distance_filed = SampleGlobalSDF(sample_world_position);
            float min_hit_distance = gloabl_sdf_voxel_size;
            if(distance_filed < min_hit_distance)
            {
                bhit = true;
                sample_ray_t = sample_ray_t + distance_filed - min_hit_distance;
                break;
            }

            sample_ray_t += distance_filed;
        }

        if(step_idx == max_step)
        {
            bhit = true;
        }

        if(bhit)
        {
            hit_result.bHit = true;
            hit_result.hit_distance = sample_ray_t;
        }
    }
};

float3 CalculateGloablSDFNormal(float3 world_pos_sample)
{
    float R = SampleGlobalSDF(float3(world_pos_sample.x + gloabl_sdf_voxel_size, world_pos_sample.y, world_pos_sample.z));
    float L = SampleGlobalSDF(float3(world_pos_sample.x - gloabl_sdf_voxel_size, world_pos_sample.y, world_pos_sample.z));

    float F = SampleGlobalSDF(float3(world_pos_sample.x, world_pos_sample.y + gloabl_sdf_voxel_size, world_pos_sample.z));
    float B = SampleGlobalSDF(float3(world_pos_sample.x, world_pos_sample.y - gloabl_sdf_voxel_size, world_pos_sample.z));

    float U = SampleGlobalSDF(float3(world_pos_sample.x, world_pos_sample.y, world_pos_sample.z + gloabl_sdf_voxel_size));
    float D = SampleGlobalSDF(float3(world_pos_sample.x, world_pos_sample.y, world_pos_sample.z - gloabl_sdf_voxel_size));

    float3 gradiance = float3(R - L, F - B, U - D);
    float gradient_length = length(gradiance);
    float3 world_normal = (gradient_length > 0.00001f) ? (gradiance / gradient_length) : 0;

	return world_normal;
}
#endif