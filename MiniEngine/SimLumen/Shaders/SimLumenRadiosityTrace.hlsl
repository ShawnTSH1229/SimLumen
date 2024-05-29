#include "SimLumenCommon.hlsl"

#define MAX_FRAME_ACCUMULATED 4

Texture2D<float> scene_card_depth : register(t3);
RWTexture2D<float3> RWTraceRadianceAtlas : register(u0);

cbuffer CBLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

float2 Hammersley16( uint Index, uint NumSamples)
{
	float E1 = frac( (float)Index / NumSamples);
	float E2 = float( ( reversebits(Index) >> 16 )) * (1.0 / 65536.0);
	return float2( E1, E2 );
}

/* 0 - 3 */
uint2 GetProbeJitter(uint IndirectLightingTemporalIndex)
{
	return Hammersley16(IndirectLightingTemporalIndex % MAX_FRAME_ACCUMULATED, MAX_FRAME_ACCUMULATED) * PROBE_TEXELS_SIZE;
}

// PDF = NoL / PI
float4 CosineSampleHemisphere( float2 E )
{
	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt(E.y);
	float SinTheta = sqrt(1 - CosTheta * CosTheta);

	float3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;

	float PDF = CosTheta * (1.0 / PI);

	return float4(H, PDF);
}

// 512 * 2048 / 256

#define RADIOSITY_TRACE_THREADGROUP_SIZE 16
[numthreads(RADIOSITY_TRACE_THREADGROUP_SIZE, RADIOSITY_TRACE_THREADGROUP_SIZE, 1)]
void LumenRadiosityDistanceFieldTracingCS(uint3 thread_index : SV_DispatchThreadID)
{
    uint2 card_idx_2d = thread_index.xy / SURFACE_CACHE_CARD_SIZE;
    uint2 sub_card_idx = thread_index.xy % SURFACE_CACHE_CARD_SIZE;
    uint2 tile_idx = sub_card_idx / SURFACE_CACHE_PROBE_TEXELS_SIZE;

    uint tile_idx_1d = tile_idx.y * (SURFACE_CACHE_CARD_SIZE / 4) + tile_idx.x;
    uint2 probe_jitter = GetProbeJitter(frame_index + tile_idx_1d);
    
    uint2 probe_atlas_pos = card_idx_2d * SURFACE_CACHE_CARD_SIZE + tile_idx * SURFACE_CACHE_PROBE_TEXELS_SIZE + probe_jitter;
    probe_atlas_pos.y = SURFACE_CACHE_TEX_SIZE - probe_atlas_pos.y;


}