#include "SimLumenCommon.hlsl"
#define PROBE_SIZE_2D 8
cbuffer CBLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

Texture2D<float3> screen_space_oct_irradiance       : register(t0);
Texture2D<float> gbuffer_depth                      : register(t1);
Texture2D<float4> gbuffer_B                         : register(t2);

RWTexture2D<float3> final_radiance                  : register(u0);

SamplerState sampler_linear_clamp                   : register(s0);

float3 GetScreenProbeIrradiance(uint2 probe_start_pos, float2 irradiance_probe_uv)
{
    float2 sub_pos = irradiance_probe_uv * (SCREEN_SPACE_PROBE - 1.0) + 1.0;
    float2 texel_uv = (probe_start_pos * SCREEN_SPACE_PROBE + sub_pos) / float2(is_pdf_thread_size_x, is_pdf_thread_size_y);
    return screen_space_oct_irradiance.SampleLevel(sampler_linear_clamp,texel_uv,0);
}

[numthreads(PROBE_SIZE_2D,PROBE_SIZE_2D,1)]
void ScreenProbeIntegrateCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID, uint3 dispatch_thread_idx: SV_DispatchThreadID)
{
    float depth = gbuffer_depth.Load(int3(dispatch_thread_idx.xy,0));
    float3 total_radiance = float3(0,0,0);
    if(depth != 0)
    {
        float3 world_normal = (gbuffer_B.Load(int3(dispatch_thread_idx.xy,0)).xyz * 2.0 - 1.0);
        float2 irradiance_probe_uv = InverseEquiAreaSphericalMapping(world_normal);

        //uint2 probe_start_pos = (dispatch_thread_idx.xy / uint2(PROBE_SIZE_2D, PROBE_SIZE_2D)) * uint2(PROBE_SIZE_2D, PROBE_SIZE_2D);
        uint2 probe_start_pos = (dispatch_thread_idx.xy / uint2(PROBE_SIZE_2D, PROBE_SIZE_2D));

        uint2 offsets[5] = {
            uint2(0,0),
            uint2(0,1),
            uint2(0,-1),
            uint2(1,0),
            uint2(-1,0),
        };

        
        for(uint index = 0; index < 5; index++)
        {
            total_radiance += GetScreenProbeIrradiance(probe_start_pos + offsets[index], irradiance_probe_uv);
        }

        total_radiance /= 5.0;
        total_radiance /= PI;
    }
    final_radiance[dispatch_thread_idx.xy] = total_radiance;
}