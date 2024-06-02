#include "SimLumenCommon.hlsl"
#define MIN_PDF_TRACE 0.1
#define PROBE_SIZE_2D 8

cbuffer CLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

StructuredBuffer<float> brdf_pdf_sh : register(t0);
Texture2D<float> gbuffer_depth      : register(t1);
Texture2D<float> light_pdf_tex      : register(t2);

RWTexture2D<uint> rwstructed_is_indirect_table: register(u0);

groupshared uint2 RaysToRefine[PROBE_SIZE_2D * PROBE_SIZE_2D * 2];
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

[numthreads(PROBE_SIZE_2D, PROBE_SIZE_2D, 1)]
void StructuredImportanceSamplingCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 dispatch_thread_idx: SV_DispatchThreadID)
{
    uint is_result = 0;

    uint2 ss_probe_idx_xy = group_idx.xy;
    uint2 ss_probe_atlas_pos = ss_probe_idx_xy * PROBE_SIZE_2D + PROBE_SIZE_2D / 2;
    float probe_depth = gbuffer_depth.Load(int3(ss_probe_atlas_pos.xy,0));
    uint uniform_level = 1;
    if(probe_depth != 0.0)
    {
        uint ss_probe_idx_1d = (ss_probe_idx_xy.y * screen_probe_size_x + ss_probe_idx_xy.x);
        uint sh_base_idx = ss_probe_idx_1d * 9;

        // brdf pdf
        FThreeBandSHVector brdf;
        brdf.V0.x = brdf_pdf_sh[sh_base_idx + 0];
		brdf.V0.y = brdf_pdf_sh[sh_base_idx + 1];
		brdf.V0.z = brdf_pdf_sh[sh_base_idx + 2];
		brdf.V0.w = brdf_pdf_sh[sh_base_idx + 3];
		brdf.V1.x = brdf_pdf_sh[sh_base_idx + 4];
		brdf.V1.y = brdf_pdf_sh[sh_base_idx + 5];
		brdf.V1.z = brdf_pdf_sh[sh_base_idx + 6];
		brdf.V1.w = brdf_pdf_sh[sh_base_idx + 7];
		brdf.V2.x = brdf_pdf_sh[sh_base_idx + 8];

        float2 probe_uv = (group_thread_idx.xy + float2(0.5f,0.5f)) / PROBE_SIZE_2D;
        float3 world_cone_direction = EquiAreaSphericalMapping(probe_uv);

        FThreeBandSHVector direction_sh = SHBasisFunction3(world_cone_direction);
        float pdf = max(DotSH3(brdf, direction_sh), 0);

        float light_pdf = light_pdf_tex.Load(int3(dispatch_thread_idx.xy,0));
        bool is_pdf_no_culled_by_brdf = pdf >= MIN_PDF_TRACE;

        float light_pdf_scaled = light_pdf * PROBE_SIZE_2D * PROBE_SIZE_2D;
        pdf *= light_pdf_scaled;
        if(is_pdf_no_culled_by_brdf)
        {
            pdf = max(pdf, MIN_PDF_TRACE);
        }

        RaysToRefine[group_thread_idx.y * PROBE_SIZE_2D + group_thread_idx.x] = PackRaySortInfo(group_thread_idx.xy, uniform_level, pdf);
        GroupMemoryBarrierWithGroupSync();

        // gpu sort
        uint sort_offset = PROBE_SIZE_2D * PROBE_SIZE_2D;
        uint thread_idx = group_thread_idx.y * PROBE_SIZE_2D + group_thread_idx.x;
        {
            uint2 ray_texel_coord;
			float sort_key;
            uint level;
			UnpackRaySortInfo(RaysToRefine[thread_idx], ray_texel_coord, level , sort_key);

            uint num_smaller = 0;
            for(uint other_ray_idx = 0; other_ray_idx < PROBE_SIZE_2D * PROBE_SIZE_2D; other_ray_idx++)
            {
                uint2 other_ray_texel_coord;
			    float other_sort_key;
                uint  other_level;
			    UnpackRaySortInfo(RaysToRefine[other_ray_idx], other_ray_texel_coord, other_level, other_sort_key);
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
        uint ray_idx_to_refine = max((int)PROBE_SIZE_2D * PROBE_SIZE_2D - (int)merge_idx - 1, 0);
        uint ray_idx_to_merge = merge_idx * 3 + 2;

        if(ray_idx_to_merge < ray_idx_to_refine)
        {
            uint2 ray_tex_coord_to_merge;
			uint ray_level_to_merge;
			float ray_pdf_to_merge;
			UnpackRaySortInfo(RaysToRefine[sort_offset + ray_idx_to_refine], ray_tex_coord_to_merge, ray_level_to_merge, ray_pdf_to_merge);

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
            uint ray_idx_to_subdivide = PROBE_SIZE_2D * PROBE_SIZE_2D - thread_idx - 1;
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
		UnpackRaySortInfo(RaysToRefine[sort_offset + thread_idx], write_ray_tex_coord, write_ray_level, write_ray_pdf);

        uint2 write_ray_coord = uint2(thread_idx % PROBE_SIZE_2D, thread_idx / PROBE_SIZE_2D);
        is_result = PackRayInfo(write_ray_tex_coord,write_ray_level);
    }
    rwstructed_is_indirect_table[dispatch_thread_idx.xy] = is_result;
}