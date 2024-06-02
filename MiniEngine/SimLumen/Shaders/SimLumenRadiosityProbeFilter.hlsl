#include "SimLumenCommon.hlsl"
Texture2D<float4> trace_radiance_atlas : register(t0);
Texture2D<float> scene_card_depth : register(t1);
RWTexture2D<float4> trace_radiance_atlas_filtered : register(u0);

//2048 / 16, 512 / 16 
#define RADIOSITY_TRACE_THREADGROUP_SIZE 16
[numthreads(RADIOSITY_TRACE_THREADGROUP_SIZE, RADIOSITY_TRACE_THREADGROUP_SIZE, 1)]
void LumenRadiositySpatialFilterProbeRadiance(uint3 thread_index : SV_DispatchThreadID)
{
    uint2 pixel_atlas_pos = thread_index.xy;
    pixel_atlas_pos.y = SURFACE_CACHE_TEX_SIZE - pixel_atlas_pos.y;

    float depth = scene_card_depth.Load(int3(pixel_atlas_pos.xy,0));
    if(depth != 0)
    {   
        float center_weight = 2;
        float3 radiance = trace_radiance_atlas.Load(int3(pixel_atlas_pos.xy,0)).xyz * center_weight;
        float total_weight = center_weight;

	    const uint num_samples = 4;
	    int2 NeighborOffsets[num_samples];
	    NeighborOffsets[0] = int2(0, 1);
	    NeighborOffsets[1] = int2(1, 0);
	    NeighborOffsets[2] = int2(0, -1);
	    NeighborOffsets[3] = int2(-1, 0);

        for(uint idx = 0; idx < num_samples; idx++)
        {
            uint2 neig_idx = NeighborOffsets[idx] + thread_index.xy;
            uint2 neig_atlas_pos = uint2(neig_idx.x, SURFACE_CACHE_TEX_SIZE - neig_idx.y);
            
            radiance += trace_radiance_atlas.Load(int3(neig_atlas_pos.xy,0)).xyz;
            total_weight += 1.0f;
        }
        trace_radiance_atlas_filtered[pixel_atlas_pos] = float4(radiance / total_weight,0.0);
    }
}