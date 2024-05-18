
#include "SimLuSceneCommon.hlsl"
#include "SimLuSHCommon.hlsl"

#define STRUCTURED_IS_GROUP_SIZE 8
#define MIN_PDF_TRACE 0.1

cbuffer SLumImportanceSamplingConstant : register(b0)
{
    uint2 screen_probe_size_xy;
    float2 gbuffer_size_and_inv;
    float screen_probe_down_sample_factor;
    float screen_probe_oct_resolution;
};

Buffer<float> BRDFPdfSH : register(t0)
Texture2D<float> probe_depth: register(t0);
RWTexture2D<float> LightPdf: register(t1);
RWTexture2D<uint> RWStructuredISIndirectTable: register(u0);

groupshared uint2 rays_to_refine[STRUCTURED_IS_GROUP_SIZE * STRUCTURED_IS_GROUP_SIZE* 2];
groupshared uint num_rays_to_subdivide;

uint2 PackRaySortInfo(uint2 texel_coord, uint Level,float pdf)
{
	return uint2((texel_coord.x & 0xFF) | ((texel_coord.y & 0xFF) << 8) | ((Level & 0xFF) << 16), asuint(pdf));
}

void UnpackRaySortInfo(uint2 ray_sort_info, out uint2 texel_coord, out uint Level, out float pdf)
{
	texel_coord.x = ray_sort_info.x & 0xFF;
	texel_coord.y = (ray_sort_info.x >> 8) & 0xFF;
    Level = (ray_sort_info.x >> 16) & 0xFF;
	pdf = asfloat(ray_sort_info.y);
}

[numthreads(STRUCTURED_IS_GROUP_SIZE, STRUCTURED_IS_GROUP_SIZE, 1)]
void ScreenProbeGenerateRaysCS(uint3 group_id : SV_GroupID, uint3 group_thread_id : SV_GroupThreadID)
{
    uint2 ss_probe_idx_xy = group_id.xy;
    uint2 thred_pix_idx_xy = group_thread_id.xy;

    float probe_depth = probe_depth.Load(ss_probe_idx_xy);//todo:check valid
    uint uniform_level = 1;
    if(probe_depth > 0)
    {
        uint ss_probe_idx_1d = ss_probe_idx_xy.y * screen_probe_size_xy.x + ss_probe_idx_xy.x;
        uint sh_base_idx = ss_probe_idx_1d * 9;

        // brdf pdf
        SThreeBandSH brdf;
        brdf.V0.x = BRDFPdfSH[sh_base_idx + 0];
		brdf.V0.y = BRDFPdfSH[sh_base_idx + 1];
		brdf.V0.z = BRDFPdfSH[sh_base_idx + 2];
		brdf.V0.w = BRDFPdfSH[sh_base_idx + 3];
		brdf.V1.x = BRDFPdfSH[sh_base_idx + 4];
		brdf.V1.y = BRDFPdfSH[sh_base_idx + 5];
		brdf.V1.z = BRDFPdfSH[sh_base_idx + 6];
		brdf.V1.w = BRDFPdfSH[sh_base_idx + 7];
		brdf.V2.x = BRDFPdfSH[sh_base_idx + 8];

        float probe_uv = (thred_pix_idx_xy.xy + float2(0.5f,0.5f)) / STRUCTURED_IS_GROUP_SIZE;
        float3 world_cone_direction = EquiAreaSphericalMapping(probe_uv);

        SThreeBandSH direction_sh = SHBasisFunction3(world_cone_direction);
		float pdf = max(DotSH3(brdf, direction_sh), 0);

        // light pdf
        float light_pdf = LightPdf.Load(int3(ss_probe_idx_xy * STRUCTURED_IS_GROUP_SIZE + thred_pix_idx_xy , 0));

        bool is_pdf_no_culled_by_brdf = pdf >= MIN_PDF_TRACE;
        float light_pdf_scaled = light_pdf * STRUCTURED_IS_GROUP_SIZE * STRUCTURED_IS_GROUP_SIZE;

        pdf *= light_pdf_scaled;
        if(is_pdf_no_culled_by_brdf)
        {
            pdf = max(pdf, MIN_PDF_TRACE);
        }

        RaysToRefine[thred_pix_idx_xy.y * STRUCTURED_IS_GROUP_SIZE + thred_pix_idx_xy.x] = PackRaySortInfo(thred_pix_idx_xy, uniform_level, pdf);

        GroupMemoryBarrierWithGroupSync();

        uint sort_offset = STRUCTURED_IS_GROUP_SIZE * STRUCTURED_IS_GROUP_SIZE;
        uint thread_idx = thred_pix_idx_xy.y * STRUCTURED_IS_GROUP_SIZE + thred_pix_idx_xy.x;
        {
            uint2 ray_texel_coord;
			float sort_key;
            uint level;
			UnpackRaySortInfo(RaysToRefine[thread_idx], ray_texel_coord, level , sort_key);

            uint num_smaller = 0;
            for(uint other_ray_idx = 0; other_ray_idx < STRUCTURED_IS_GROUP_SIZE * STRUCTURED_IS_GROUP_SIZE)
            {
                uint2 other_ray_texel_coord;
			    float other_sort_key;
                uint  other_level;
			    UnpackRaySortInfo(RaysToRefine[other_ray_idx], pther_ray_texel_coord, other_level, pther_sort_key);

                if(other_sort_key < sort_key || (other_sort_key == sort_key && other_ray_idx < thread_idx))
                {
                    num_smaller++;
                }
            }

            RaysToRefine[num_smaller + sort_offset] = RaysToRefine[thread_idx];
        }

        if(thread_idx == 0)
        {
            num_rays_to_subdivide = 0;
        }

        GroupMemoryBarrierWithGroupSync();

        uint merge_thread_idx = thread_idx % 3;
        uint merge_idx = thread_idx / 3;

        uint ray_idx_to_refine = max((int)STRUCTURED_IS_GROUP_SIZE * STRUCTURED_IS_GROUP_SIZE - (int)merge_idx - 1, 0);
        uint ray_idx_to_merge = merge_idx * 3 + 2;

        if(ray_idx_to_merge < ray_idx_to_refine)
        {
            uint2 ray_tex_coord_to_merge;
			uint ray_level_to_merge;
			float ray_pdf_to_merge;
			UnpackRaySortInfo(RaysToRefine[sort_offset + ray_tex_coord_to_merge], ray_tex_coord_to_merge, ray_level_to_merge, ray_pdf_to_merge);

            if(ray_pdf_to_merge < MIN_PDF_TRACE)
            {
                uint2 origin_ray_tex_coord;
                uint original_ray_level;
                uint original_pdf;
                UnpackRaySortInfo(RaysToRefine[sort_offset + ray_idx_to_refine], origin_ray_tex_coord, original_ray_level, original_pdf);

                RaysToRefine[sort_offset + thread_idx] = PackRaySortInfo(origin_ray_tex_coord * 2 + uint2((merge_thread_idx + 1) % 2, (merge_thread_idx + 1) / 2), original_ray_level - 1, 0.0f);

				if (merge_idx == 0)
				{
					InterlockedAdd(num_rays_to_subdivide, 1);
				}
            }
        }

        GroupMemoryBarrierWithGroupSync();

        if (thread_idx < num_rays_to_subdivide)
		{
			uint ray_idx_to_subdivide = STRUCTURED_IS_GROUP_SIZE * STRUCTURED_IS_GROUP_SIZE - thread_idx - 1;

			uint2 origin_ray_tex_coord;
			uint original_ray_level;
			float original_pdf;
			UnpackRaySortInfo(RaysToRefine[sort_offset + ray_idx_to_subdivide], origin_ray_tex_coord, original_ray_level, original_pdf);

			RaysToRefine[sort_offset + ray_idx_to_subdivide] = PackRaySortInfo(origin_ray_tex_coord * 2, original_ray_level - 1, 0.0f);
		}

        GroupMemoryBarrierWithGroupSync();

        uint2 write_ray_tex_coord;
		uint write_ray_level;
		float write_ray_pdf;
		UnpackRaySortInfo(RaysToRefine[sort_offset + thread_idx], write_ray_tex_coord, write_level, write_ray_pdf);

		uint2 write_ray_coord = uint2(thread_idx % STRUCTURED_IS_GROUP_SIZE, thread_idx / STRUCTURED_IS_GROUP_SIZE);

        uint write_buffer_idx = screen_probe_size_xy * ss_probe_idx_xy + write_ray_coord;
        RWStructuredISIndirectTable[write_buffer_idx] = PackRayInfo(write_ray_tex_coord,write_ray_level);
    }

}