#include "SimLumenCommon.hlsl"
#define PROBE_SIZE_2D 8

Texture2D<float> gbuffer_depth      : register(t0);
Texture2D<uint> structed_is_indirect_table: register(t1);
Texture2D<float3> screen_space_trace_radiance : register(t2);

RWTexture2D<float3> screen_space_radiance: register(u0);

groupshared uint shared_accumulator[PROBE_SIZE_2D * PROBE_SIZE_2D][3];

[numthreads(PROBE_SIZE_2D,PROBE_SIZE_2D,1)]
void ScreenProbeCompositeCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 dispatch_thread_idx: SV_DispatchThreadID)
{
    uint2 ss_probe_idx_xy = group_idx.xy;
    uint2 ss_probe_atlas_pos = ss_probe_idx_xy * PROBE_SIZE_2D + PROBE_SIZE_2D / 2;
    float probe_depth = gbuffer_depth.Load(int3(ss_probe_atlas_pos.xy,0));

    float3 radiance = float3(0,0,0);
    if(probe_depth != 0.0)
    {
        uint thread_index = group_thread_idx.y * PROBE_SIZE_2D + group_thread_idx.x;
		shared_accumulator[thread_index][0] = 0;
		shared_accumulator[thread_index][1] = 0;
		shared_accumulator[thread_index][2] = 0;

        GroupMemoryBarrierWithGroupSync();

        float max_ray_intensity = 5.0f;
        float max_value_perthread = (float)0xFFFFFFFF / ((float)8 * 8);
        float lighting_quantize_scale = max_value_perthread / max_ray_intensity;

        {
            uint packed_ray_info = structed_is_indirect_table.Load(int3(dispatch_thread_idx.xy,0));

            uint2 texel_coord;
            uint level;
            UnpackRayInfo(packed_ray_info, texel_coord, level);

            uint mip_size = 16 >> level;

            const float sample_weight = (float)8 / mip_size * 8 / mip_size; // level 0: weight 1/4, level 1: weight 1
            float3 lighting = screen_space_trace_radiance.Load(int3(dispatch_thread_idx.xy, 0)).xyz * sample_weight;
            float max_lighting = max(lighting.x, max(lighting.y, lighting.z));
            
            if(max_lighting > max_ray_intensity)
            {
                 lighting *= (max_ray_intensity / max_lighting);
            }

            uint2 remaped_texel_coord = texel_coord * PROBE_SIZE_2D / mip_size;
            uint remapped_thread_idx = remaped_texel_coord.y * PROBE_SIZE_2D + remaped_texel_coord.x;


            uint3 quantized_lighting = lighting * lighting_quantize_scale;

            InterlockedAdd(shared_accumulator[remapped_thread_idx][0], quantized_lighting.x);
		    InterlockedAdd(shared_accumulator[remapped_thread_idx][1], quantized_lighting.y);
		    InterlockedAdd(shared_accumulator[remapped_thread_idx][2], quantized_lighting.z);
        }

        GroupMemoryBarrierWithGroupSync();

        {
            uint thread_index = group_thread_idx.y * PROBE_SIZE_2D + group_thread_idx.x;
            radiance = float3(shared_accumulator[thread_index][0], shared_accumulator[thread_index][1], shared_accumulator[thread_index][2]) / lighting_quantize_scale;
        }
    }

    screen_space_radiance[dispatch_thread_idx.xy] = radiance;
}