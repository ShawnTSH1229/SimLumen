#include "SimLumenCommon.hlsl"

#define PROBE_SIZE_2D 8

#ifndef ENABLE_VIS_BRDF_PDF
#define ENABLE_VIS_BRDF_PDF 0
#endif

cbuffer CLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

cbuffer CLumenViewInfo : register(b1)
{
    GLOBAL_VIEW_CONSTANT_BUFFER
};

Texture2D<float4> gbuffer_b         : register(t0);
Texture2D<float> gbuffer_depth      : register(t1);
Texture2D<float4> gbuffer_c         : register(t2);

RWStructuredBuffer<float> brdf_pdf_sh : register(u0);
#if ENABLE_VIS_BRDF_PDF
RWTexture2D<float> brdf_pdf_visualize : register(u1);
#endif

groupshared float pdf_sh[PROBE_SIZE_2D * PROBE_SIZE_2D * 2][9];
groupshared uint group_num_sh;

FThreeBandSHVector GetGroupSharedSH(uint ThreadIndex)
{
    FThreeBandSHVector brdf;
    brdf.V0.x = pdf_sh[ThreadIndex][0];
    brdf.V0.y = pdf_sh[ThreadIndex][1];
    brdf.V0.z = pdf_sh[ThreadIndex][2];
    brdf.V0.w = pdf_sh[ThreadIndex][3];
    brdf.V1.x = pdf_sh[ThreadIndex][4];
    brdf.V1.y = pdf_sh[ThreadIndex][5];
    brdf.V1.z = pdf_sh[ThreadIndex][6];
    brdf.V1.w = pdf_sh[ThreadIndex][7];
    brdf.V2.x = pdf_sh[ThreadIndex][8];
    return brdf;
}

void WriteGroupSharedSH(FThreeBandSHVector SH, uint ThreadIndex)
{
	pdf_sh[ThreadIndex][0] = SH.V0.x;
	pdf_sh[ThreadIndex][1] = SH.V0.y;
	pdf_sh[ThreadIndex][2] = SH.V0.z;
	pdf_sh[ThreadIndex][3] = SH.V0.w;
	pdf_sh[ThreadIndex][4] = SH.V1.x;
	pdf_sh[ThreadIndex][5] = SH.V1.y;
	pdf_sh[ThreadIndex][6] = SH.V1.z;
	pdf_sh[ThreadIndex][7] = SH.V1.w;
	pdf_sh[ThreadIndex][8] = SH.V2.x;
}

float3 GetWorldPosByDepth(float depth, float2 tex_uv)
{
    float4 ndc = float4( tex_uv.xy * 2.0 - 1.0, depth, 1.0f );
    ndc.y = (ndc.y * (-1.0));
	float4 wp = mul(InverseViewProjMatrix, ndc);
	float3 world_position =  wp.xyz / wp.w;
    return world_position;
}

[numthreads(PROBE_SIZE_2D, PROBE_SIZE_2D, 1)]
void BRDFPdfCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 dispatch_thread_idx: SV_DispatchThreadID)
{
    uint2 ss_probe_idx_xy = group_idx.xy;
    uint2 ss_probe_atlas_pos = ss_probe_idx_xy * PROBE_SIZE_2D + PROBE_SIZE_2D / 2;
    float probe_depth = gbuffer_depth.Load(int3(ss_probe_atlas_pos.xy,0));

    uint thread_index = group_thread_idx.y * PROBE_SIZE_2D + group_thread_idx.x;
    if (thread_index < 9)
    {
        uint write_index = (ss_probe_idx_xy.y * screen_probe_size_x + ss_probe_idx_xy.x) * 9 + thread_index;
        brdf_pdf_sh[write_index] = 0.0;
    }

    const float2 global_thread_size = float2(is_pdf_thread_size_x,is_pdf_thread_size_x);
    uint offset = 0;

    if (thread_index == 0)
	{
		group_num_sh = 0;
	}
    GroupMemoryBarrierWithGroupSync();

    if(probe_depth != 0.0)
    {
        float thread_depth = gbuffer_depth.Load(int3(dispatch_thread_idx.xy, 0 ));
        if(thread_depth != 0.0 && dispatch_thread_idx.x < is_pdf_thread_size_x && dispatch_thread_idx.y < is_pdf_thread_size_y)
        {
            float3 thread_world_normal = gbuffer_b.Load(int3(dispatch_thread_idx.xy, 0)).xyz * 2.0 - 1.0;
            float2 piexl_tex_uv = float2(dispatch_thread_idx.xy) / global_thread_size;

            //todo: fixme
            //float3 pixel_world_position = GetWorldPosByDepth(thread_depth, piexl_tex_uv);
            //float3 probe_world_position = GetWorldPosByDepth(probe_depth, ss_probe_atlas_pos / global_thread_size);

            float3 pixel_world_position = gbuffer_c.Load(int3(dispatch_thread_idx.xy,0)).xyz;
            float3 probe_world_position = gbuffer_c.Load(int3(ss_probe_atlas_pos.xy,0)).xyz;

            float4 pixel_world_plane = float4(thread_world_normal, dot(thread_world_normal,pixel_world_position));
            float plane_distance = abs(dot(float4(probe_world_position, -1), pixel_world_plane));

            float probe_view_dist = length(probe_world_position - CameraPos);
            float relative_depth_diff = plane_distance / probe_view_dist;
            float depth_weight = exp2(-10000.0f * (relative_depth_diff * relative_depth_diff));
            if(depth_weight > 0.1f)
            {
                uint write_index;
			    InterlockedAdd(group_num_sh, 1, write_index);

                FThreeBandSHVector brdf = CalcDiffuseTransferSH3(thread_world_normal, 1.0);
                WriteGroupSharedSH(brdf, write_index);
            }
        }
        GroupMemoryBarrierWithGroupSync();

        uint num_sh_to_accumulate = group_num_sh;

        while (num_sh_to_accumulate > 1)
        {
            uint thead_base_index = thread_index * 4;
            if (thead_base_index < num_sh_to_accumulate)
            {
                FThreeBandSHVector PDF = GetGroupSharedSH(thead_base_index + offset);

                if (thead_base_index + 1 < num_sh_to_accumulate)
                {
                    PDF = AddSH(PDF, GetGroupSharedSH(thead_base_index + 1 + offset));
                }

                if (thead_base_index + 2 < num_sh_to_accumulate)
				{
					PDF = AddSH(PDF, GetGroupSharedSH(thead_base_index + 2 + offset));
				}

                if (thead_base_index + 3 < num_sh_to_accumulate)
				{
					PDF = AddSH(PDF, GetGroupSharedSH(thead_base_index + 3 + offset));
				}

                WriteGroupSharedSH(PDF, thread_index + offset + num_sh_to_accumulate);
            }
            offset += num_sh_to_accumulate;
            num_sh_to_accumulate = (num_sh_to_accumulate + 3) / 4;
            GroupMemoryBarrierWithGroupSync();
        }

        if (thread_index < 9 && group_num_sh > 0)
        {
            uint write_index = (ss_probe_idx_xy.y * screen_probe_size_x + ss_probe_idx_xy.x) * 9 + thread_index;
            float normalize_weight = 1.0f / (float)(group_num_sh);
            brdf_pdf_sh[write_index] = pdf_sh[offset][thread_index] * normalize_weight;
        }
    }

#if ENABLE_VIS_BRDF_PDF
    GroupMemoryBarrierWithGroupSync();
    if(dispatch_thread_idx.x < is_pdf_thread_size_x && dispatch_thread_idx.y < is_pdf_thread_size_y)
    {
        brdf_pdf_visualize[dispatch_thread_idx.xy] = 0.0f;
    }
    GroupMemoryBarrierWithGroupSync();
    if(dispatch_thread_idx.x < is_pdf_thread_size_x && dispatch_thread_idx.y < is_pdf_thread_size_y && group_num_sh > 0)
    {
        FThreeBandSHVector brdf;
		float normalize_weight = 1.0f / (float)(group_num_sh);
		brdf.V0.x = pdf_sh[offset][0] * normalize_weight;
		brdf.V0.y = pdf_sh[offset][1] * normalize_weight;
		brdf.V0.z = pdf_sh[offset][2] * normalize_weight;
		brdf.V0.w = pdf_sh[offset][3] * normalize_weight;
		brdf.V1.x = pdf_sh[offset][4] * normalize_weight;
		brdf.V1.y = pdf_sh[offset][5] * normalize_weight;
		brdf.V1.z = pdf_sh[offset][6] * normalize_weight;
		brdf.V1.w = pdf_sh[offset][7] * normalize_weight;
		brdf.V2.x = pdf_sh[offset][8] * normalize_weight;

        float2 probe_texel_center = float2(0.5, 0.5);
        float2 probe_uv = float2(group_thread_idx.xy + probe_texel_center) / (float)SCREEN_SPACE_PROBE;
        float3 world_cone_direction = EquiAreaSphericalMapping(probe_uv);


        FThreeBandSHVector direction_sh = SHBasisFunction3(world_cone_direction);
		float pdf = max(DotSH3(brdf, direction_sh), 0);
        brdf_pdf_visualize[dispatch_thread_idx.xy] = pdf;
    }
#endif
}