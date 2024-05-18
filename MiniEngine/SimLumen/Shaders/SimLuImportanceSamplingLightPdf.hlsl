#define LIGHT_PDF_GROUP_SIZE 8

//https://fileadmin.cs.lth.se/graphics/research/papers/2008/simdmapping/clarberg_simdmapping08_preprint.pdf

groupshared float group_light_pdf[LIGHT_PDF_GROUP_SIZE * LIGHT_PDF_GROUP_SIZE * 4];

cbuffer SLumImportanceSamplingConstant : register(b0)
{
    uint2 screen_probe_size_xy;
    float2 gbuffer_size_and_inv;
    float screen_probe_down_sample_factor;
    float screen_probe_oct_resolution;
};

RWTexture2D<float> RWLightPdf;
Texture2D<float> probe_depth: register(t0);

[numthreads(LIGHT_PDF_GROUP_SIZE, LIGHT_PDF_GROUP_SIZE, 1)]
void LightPdfCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID)
{
    uint2 ss_probe_idx_xy = group_idx.xy;
    float probe_depth = probe_depth.Load(ss_probe_idx_xy);//todo:check valid

    if(probe_depth > 0)
    {
        float3 probe_world_pos = ;
        float pdf = 0.0;
        float3 lighting = 0;
        float transparency = 1.0;
#if PROBE_RADIANCE_HISTORY

#endif

        if(transparency > 0.0f)
        {
#if PROBE_RADIANCE_HISTORY
            lighting = 1;
#endif            
        }

        pdf = Luminance(lighting);
		group_light_pdf[group_thread_idx.y * screen_probe_oct_resolution + group_thread_idx.x] = PDF;

        GroupMemoryBarrierWithGroupSync();

        uint thread_index = group_thread_idx.y * screen_probe_oct_resolution + group_thread_idx.x;
        uint num_value_to_accumulate = screen_probe_oct_resolution * screen_probe_oct_resolution;
        uint offset = 0;

        while (num_value_to_accumulate > 1)
		{
			uint thread_base_index = thread_index * 4;

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

				group_light_pdf[ThreadIndex + offset + num_value_to_accumulate] = local_pdf;
			}

			offset += num_value_to_accumulate;
			num_value_to_accumulate = (num_value_to_accumulate + 3) / 4;

			GroupMemoryBarrierWithGroupSync();
        }

        float pdf_sum = group_light_pdf[offset];
        RWLightPdf[ss_probe_idx_xy * screen_probe_oct_resolution + group_thread_idx] = pdf / max(pdf_sum, 0.0001f);
    }
}