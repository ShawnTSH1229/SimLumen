#include "SimLumenCommon.hlsl"
#define PROBE_SIZE_2D 8

cbuffer CLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};


Texture2D<float> gbuffer_depth      : register(t0);
Texture2D<float3> screen_space_radiance : register(t1);

RWTexture2D<float3> screen_space_filtered_radiance : register(u0);

[numthreads(PROBE_SIZE_2D,PROBE_SIZE_2D,1)]
void ScreenProbeFilterCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 dispatch_thread_idx: SV_DispatchThreadID)
{
    uint2 ss_probe_idx_xy = group_idx.xy;
    uint2 ss_probe_atlas_pos = ss_probe_idx_xy * PROBE_SIZE_2D + PROBE_SIZE_2D / 2;
    float probe_depth = gbuffer_depth.Load(int3(ss_probe_atlas_pos.xy,0));

    float3 filtered_radiance = float3(0,0,0);
    if(probe_depth != 0.0)
    {
        float3 total_radiance = screen_space_radiance.Load(int3(dispatch_thread_idx.xy, 0)).xyz;
        float total_weight = 1.0;

        int2 offsets[4];
		offsets[0] = int2(-1, 0);
		offsets[1] = int2(1, 0);
		offsets[2] = int2(0, -1);
		offsets[3] = int2(0, 1);

        for (uint offset_index = 0; offset_index < 4; offset_index++)
        {
            int2 neighbor_index = offsets[offset_index] * PROBE_SIZE_2D + dispatch_thread_idx.xy;
            if((neighbor_index.x >= 0) && (neighbor_index.x < is_pdf_thread_size_x) && (neighbor_index.y >= 0) && (neighbor_index.y < is_pdf_thread_size_y))
            {
                float neigh_depth = gbuffer_depth.Load(int3(ss_probe_atlas_pos.xy + offsets[offset_index] * PROBE_SIZE_2D, 0));
                if(neigh_depth != 0)
                {
                    total_radiance += screen_space_radiance.Load(int3(dispatch_thread_idx.xy, 0)).xyz;
                    total_weight += 1.0;
                }
            }
        }

        filtered_radiance = total_radiance / total_weight;
    }

    screen_space_filtered_radiance[dispatch_thread_idx.xy] = filtered_radiance;
}