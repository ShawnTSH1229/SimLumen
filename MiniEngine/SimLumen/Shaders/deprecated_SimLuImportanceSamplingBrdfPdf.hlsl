#include "SimLuShCommon.hlsl"

#define PROBE_SIZE_2D 8
#define PROBE_SIZE_1D 64

cbuffer SLumImportanceSamplingConstant : register(b0)
{
    uint2 screen_probe_size_xy;
    float2 gbuffer_size_and_inv;
    float screen_probe_down_sample_factor;
	float screen_probe_oct_resolution;
};


RWBuffer<float> BRDFPdfSH : register(u0);

Texture2D<float> probe_depth: register(t0);

//Texture2D<float> gbuffer_A          : register(t0); // base color and metalic
Texture2D<float> gbuffer_B          : register(t0); // world normal and roughness
Texture2D<float> gbuffer_depth      : register(t0);

groupshared float pdf_sh[PROBE_SIZE_2D * PROBE_SIZE_2D * 2][9];
groupshared uint group_num_sh;

SThreeBandSH GetGroupSharedSH(uint ThreadIndex)
{
	SThreeBandSH BRDF;
	BRDF.V0.x = pdf_sh[ThreadIndex][0];
	BRDF.V0.y = pdf_sh[ThreadIndex][1];
	BRDF.V0.z = pdf_sh[ThreadIndex][2];
	BRDF.V0.w = pdf_sh[ThreadIndex][3];
	BRDF.V1.x = pdf_sh[ThreadIndex][4];
	BRDF.V1.y = pdf_sh[ThreadIndex][5];
	BRDF.V1.z = pdf_sh[ThreadIndex][6];
	BRDF.V1.w = pdf_sh[ThreadIndex][7];
	BRDF.V2.x = pdf_sh[ThreadIndex][8];
	return BRDF;
}

void WriteGroupSharedSH(SThreeBandSH SH, uint ThreadIndex)
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

[numthreads(PROBE_SIZE_2D, PROBE_SIZE_2D, 1)]
void BRDFPdfCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID)
{
    uint2 ss_probe_idx_xy = group_idx.xy;
    float probe_depth = probe_depth.Load(ss_probe_idx_xy);//todo:check valid

	uint thread_index = group_thread_idx.y * PROBE_SIZE_2D + group_thread_idx.x;

    if(probe_depth > 0)
    {
		if (thread_index == 0)
		{
			group_num_sh = 0;
		}

        GroupMemoryBarrierWithGroupSync();

        float2 ss_probe_screen_pos = ss_probe_idx_xy * screen_probe_down_sample_factor; //
        float2 thread_offset = group_thread_idx.xy / (float)PROBE_SIZE_2D * screen_probe_down_sample_factor;
        float2 thread_screen_pos = ss_probe_screen_pos + thread_offset;
        float2 thread_screen_uv = clamp(thread_screen_pos * screen_probe_down_sample_factor.zw,float2(0.0,0.0),float2(1.0,1.0));

        float mat_depth = gbuffer_depth.Sample(g_point_sampler, thread_screen_uv).x;
        float3 mat_world_normal = gbuffer_B.Sample(g_point_sampler, thread_screen_uv).xyzs;

        float3 pixel_world_pos = ;

        float4 pixel_plane = float4(mat_world_normal,dot(mat_world_normal,pixel_world_pos));
        float3 probe_world_pos = ;

        float plane_distance = abs(dot(float4(probe_world_pos, -1), pixel_plane));
        float relative = depth_diff = plane_distance / probe_depth;
		float depth_weight = exp2(-10000.0f * (relative * relative));

        if(depth_weight > 1.0f)
        {
            uint index;
			InterlockedAdd(group_num_sh, 1, index);

            SThreeBandSH brdf = CalcDiffuseTransferSH3(mat_world_normal,1.0);
            WriteGroupSharedSH(brdf,index);
        }

        GroupMemoryBarrierWithGroupSync();

        uint num_sh_to_accumulate = group_num_sh;
        uint offset = 0;

        while (num_sh_to_accumulate > 1)
        {
            uint thead_base_index = thread_index * 4;

			if (thead_base_index < num_sh_to_accumulate)
			{
				SThreeBandSH PDF = GetGroupSharedSH(thead_base_index + offset);

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

				WriteGroupSharedSH(PDF, ThreadIndex + offset + num_sh_to_accumulate);
			}

			offset += num_sh_to_accumulate;
			num_sh_to_accumulate = (num_sh_to_accumulate + 3) / 4;

			GroupMemoryBarrierWithGroupSync();
        }

        if (thread_index < 9)
		{
			uint WriteIndex = (ss_probe_idx_xy.y * screen_probe_size_xy.x + ss_probe_idx_xy.x) * 9 + thread_index;
			float normalize_weight = 1.0f / (float)(group_num_sh);
			BRDFPdfSH[WriteIndex] = pdf_sh[offset][thread_index] * normalize_weight;
		}
    }
}
