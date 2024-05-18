#define LIGHT_PROBE_TEX_SIZE 8




// 
// (256 * 256) * 64 * 6 (light dirction)
//  x : 256 * 8 y : 256 * 8 z: 256
Texture3D<float3> voxel_lighting : register(t0);


Texture3D<float> global_sdf : register(t1);
Texture2D<uint> StructuredISIndirectTable: register(t0);
SamplerState g_sampler_3d_point: register(s0);
Texture2D<float> probe_depth : register(t0);
Buffer<uint> probe_num_per_trace_type: register(t0); 
Buffer<uint2> voxel_trace_probe_idx: register(t1);
RWTexture2D<float3> rw_trace_radiance:register(u0);

struct SGloablSDFHitResult
{
    bool bHit;
    float hit_distance;
};

cbuffer CSimLuGlobalSDFAndVoxelInfo : register(b0)
{
    float3 gloabl_sdf_center; // world space
    float3 gloabl_sdf_extents; // world space
    float global_sdf_texel_size; // 1.0 world space
    float global_distance_scale; // 0 - 1 to world space

    float3 scene_voxel_center;
    float3 scene_voxel_extents;
    float scene_voxel_size; // 1.0 world space
};

float SampleGlobalSDF(float3 sample_position_volume_space)
{
    float3 global_sdf_offsets = sample_position_volume_space / global_sdf_texel_size;
    float3 global_sdf_uvw = global_sdf_offsets / float3(GLOBAL_SDF_SIZE_X,GLOBAL_SDF_SIZE_Y,GLOBAL_SDF_SIZE_Z);
    float distance = distance_field_brick_tex.Sample(g_sampler_point_3d,global_texel_uvw); // todo: fix me 
    return distance * mesh_sdf_info.global_distance_scale.x + global_distance_scale.y;
}


void TraceGlobalSDF(float3 world_ray_start, float3 world_ray_direction, inout SGloablSDFHitResult hit_result)
{
    hit_result.bHit = false;

    float3 global_sdf_center_distance = abs(world_ray_start - gloabl_sdf_center);
    if(all(global_sdf_center_distance < gloabl_sdf_extents)) //todo: probe outside gloabl sdf
    {
        float3 global_sdf_min_pos = gloabl_sdf_center - gloabl_sdf_extents;
        
        float3 volume_ray_start = world_ray_start - global_sdf_min_pos;
        float3 volume_ray_direction = world_ray_direction;

        float sample_ray_t = -1.0f;

        uint max_step = 64;
        bool bhit = false;
        uint step_idx = 0;
        for( ; step_idx < max_step; step_idx++)
        {
            float3 sample_volume_position = volume_ray_start + volume_ray_direction * sample_ray_t;

            if(any(sample_volume_position) > gloabl_sdf_extents * 2.0)
            {
                bhit = false;
                break;
            }

            float distance_filed = SampleDistanceFieldBrickTexture(sample_volume_position);
            
            float min_hit_distance = global_sdf_texel_size * 1.0; //todo: this must be accurate
             if(distance_filed < min_hit_distance)
             {
                hit_result.bhit = true;
                sample_ray_t = sample_ray_t + distance_filed - min_hit_distance;
                break;
             }
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
}

[numthreads(LIGHT_PROBE_TEX_SIZE, LIGHT_PROBE_TEX_SIZE, 1)]
void LightTraceVoxel(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID)
{
    uint probe_trace_voxel_num = probe_num_per_trace_type[1];
    if(group_idx.x < probe_trace_voxel_num)
    {
        uint2 vox_probe_idx = voxel_trace_probe_idx[group_idx.x];
        float probe_depth = probe_depth.Load(vox_probe_idx);

        uint2 pixel_idx_xy = vox_probe_idx * LIGHT_PROBE_TEX_SIZE + group_thread_idx.xy;

        float3 probe_world_position = ;
        uint ray_info = StructuredISIndirectTable.Load(pixel_idx_xy);

        float2 probe_uv;
        GetProbeTracingUV(ray_info,probe_uv);
        float3 ray_direction = EquiAreaSphericalMapping(probe_uv);

        float3 global_sdf_center_distance = abs(probe_world_position - gloabl_sdf_center);
        if(all(global_sdf_center_distance < gloabl_sdf_extents)) //todo: probe outside gloabl sdf
        {
            SGloablSDFHitResult hit_result;
            TraceGlobalSDF(probe_world_position,ray_direction,hit_result);
            if(hit_result.bHit)
            {
                float3 hit_world_position = hit_result.hit_distance * ray_direction + probe_world_position;
                
                //
                float scene_voxel_distance = abs(hit_world_position - scene_voxel_center);
                if(all(scene_voxel_distance < scene_voxel_extents))
                {
                    // cacl volume normal
                    float3 global_sdf_min_pos = gloabl_sdf_center - gloabl_sdf_extents;
                    float3 sample_volume_position = hit_world_position - global_sdf_min_pos;
                    float sdf_voxel_offset = global_sdf_texel_size;

                    float R = SampleGlobalSDF(float3(sample_volume_position.x + sdf_voxel_offset, sample_volume_position.y, sample_volume_position.z));
                    float L = SampleGlobalSDF(float3(sample_volume_position.x - sdf_voxel_offset, sample_volume_position.y, sample_volume_position.z));
                    float F = SampleGlobalSDF(float3(sample_volume_position.x, sample_volume_position.y + sdf_voxel_offset, sample_volume_position.z));
                    float B = SampleGlobalSDF(float3(sample_volume_position.x, sample_volume_position.y + sdf_voxel_offset, sample_volume_position.z));
                    float U = SampleGlobalSDF(float3(sample_volume_position.x, sample_volume_position.y, sample_volume_position.z + sdf_voxel_offset));
                    float D = SampleGlobalSDF(float3(sample_volume_position.x, sample_volume_position.y, sample_volume_position.z + sdf_voxel_offset));

                    float3 gradiant = float3(R - L, F - B, U - D);
                    float gradient_length = length(gradiant);
                    float3 volume_normal = gradient_length > 0.00001f ? gradiant / gradient_length : 0;

                    float3 scene_voxel_min_pos = scene_voxel_center - scene_voxel_extents;
                    float3 scene_voxel_position = hit_world_position - scene_voxel_min_pos;
                    int3 voxel_index = scene_voxel_position / scene_voxel_size;
                    
                    int voxel_slice_index_z = voxel_index.z;
                    
                    int voxel_texture_pos_y = v(oxel_slice_index_z / VOXEL_SLICE_SIZE_X) * SCENE_VOXEL_SIZE_X;
                    int voxel_texture_pos_x = (voxel_slice_index_z % VOXEL_SLICE_SIZE_X) * SCENE_VOXEL_SIZE_Y;

                    voxel_texture_pos_y += voxel_index.y;
                    voxel_texture_pos_x += voxel_index.x;

                    int max_dir = 0;
                    float max_norm = 0.0;

                    float3 volume_normal_abs = abs(volume_normal);
                    if(volume_normal_abs.x > max_norm)
                    {
                        max_norm = volume_normal_abs.x;
                        max_dir = volume_normal.x > 0 ? 0 : 1;
                    }

                    if(volume_normal_abs.y > max_norm)
                    {
                        max_norm = volume_normal_abs.y;
                        max_dir = volume_normal.y > 0 ? 2 : 3;
                    }

                    if(volume_normal_abs.z > max_norm)
                    {
                        max_norm = volume_normal_abs.z;
                        max_dir = volume_normal.z > 0 ? 4 : 5;
                    }

                    uint3 voxel_texture_pos = uint3(voxel_texture_pos_x, voxel_texture_pos_y,max_dir);
                    float3 uvw = voxel_texture_pos / float3(VOXEL_TEXTURE_SIZE_X,VOXEL_TEXTURE_SIZE_Y,VOXEL_TEXTURE_SIZE_Z);
                    float3 lighting = voxel_lighting.Sample(g_sampler_point_3d,uvw);
                    rw_trace_radiance[pixel_idx_xy] = final_light;
                }
            }
        }
    }
}


