#include "SimLumenCommon.hlsl"

#define PROBE_SIZE_2D 8

#ifndef PROBE_RADIANCE_HISTORY
#define PROBE_RADIANCE_HISTORY 0
#endif

groupshared float group_light_pdf[PROBE_SIZE_2D * PROBE_SIZE_2D * 4];

Texture2D<float> gbuffer_depth      : register(t0);
RWTexture2D<float> out_light_pdf         : register(u0);

[numthreads(PROBE_SIZE_2D, PROBE_SIZE_2D, 1)]
void LightingPdfCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 dispatch_thread_idx: SV_DispatchThreadID)
{
    uint2 ss_probe_idx_xy = group_idx.xy;
    uint2 ss_probe_atlas_pos = ss_probe_idx_xy * PROBE_SIZE_2D + PROBE_SIZE_2D / 2;
    float probe_depth = gbuffer_depth.Load(int3(ss_probe_atlas_pos.xy,0));

    float light_pdf = 0;
    if(probe_depth != 0.0)
    {
        float thread_depth = gbuffer_depth.Load(int3(dispatch_thread_idx.xy, 0 ));

        float pdf = 0.0;
        float3 lighting = 0;
        float transparency = 1.0;

        if(transparency > 0.0f)
        {
#if PROBE_RADIANCE_HISTORY
#else
            lighting = 1;
#endif
        }

        pdf = Luminance(lighting);
        group_light_pdf[group_thread_idx.y * PROBE_SIZE_2D + group_thread_idx.x] = pdf;

        GroupMemoryBarrierWithGroupSync();

        uint grp_thread_index_1d = group_thread_idx.y * PROBE_SIZE_2D + group_thread_idx.x;
        uint num_value_to_accumulate = PROBE_SIZE_2D * PROBE_SIZE_2D;
        uint offset = 0;

        while (num_value_to_accumulate > 1)
		{
			uint thread_base_index = grp_thread_index_1d * 4;

			if (thread_base_index < num_value_to_accumulate)
			{
				float local_pdf = group_light_pdf[thread_base_index + offset];

				if (thread_base_index + 1 < num_value_to_accumulate)
				{
					local_pdf += group_light_pdf[thread_base_index + 1 + offset];
				}

				if (thread_base_index + 2 < num_value_to_accumulate)
				{
					local_pdf += group_light_pdf[thread_base_index + 2 + offset];
				}

				if (thread_base_index + 3 < num_value_to_accumulate)
				{
					local_pdf += group_light_pdf[thread_base_index + 3 + offset];
				}

				group_light_pdf[grp_thread_index_1d + offset + num_value_to_accumulate] = local_pdf;
			}

			offset += num_value_to_accumulate;
			num_value_to_accumulate = (num_value_to_accumulate + 3) / 4;

			GroupMemoryBarrierWithGroupSync();
        }

        float pdf_sum = group_light_pdf[offset];
        light_pdf = pdf / max(pdf_sum, 0.0001f);
    }
    out_light_pdf[dispatch_thread_idx.xy] = light_pdf;
}