#include "SimLumenCommon.hlsl"
cbuffer CBLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

StructuredBuffer<SCardInfo> scene_card_infos : register(t0);
Texture2D<float4> scene_card_albedo : register(t1);
Texture2D<float4> scene_card_normal : register(t2);
Texture2D<float> scene_card_depth : register(t3);
Texture2D<float4> radiosity_probe_sh_red_atlas : register(t4);
Texture2D<float4> radiosity_probe_sh_green_atlas : register(t5);
Texture2D<float4> radiosity_probe_sh_blue_atlas : register(t6);

RWTexture2D<float4> surface_cache_indirect_lighting : register(u0);

FTwoBandSHVectorRGB GetRadiosityProbeSH(uint2 RadiosityProbeAtlasCoord)
{
	float4 sh_red = radiosity_probe_sh_red_atlas.Load(int3(RadiosityProbeAtlasCoord, 0));
	float4 sh_green = radiosity_probe_sh_green_atlas.Load(int3(RadiosityProbeAtlasCoord, 0));
	float4 sh_blue = radiosity_probe_sh_blue_atlas.Load(int3(RadiosityProbeAtlasCoord, 0));

	FTwoBandSHVectorRGB irradiance_sh;
	irradiance_sh.R.V = sh_red;
	irradiance_sh.G.V = sh_green;
	irradiance_sh.B.V = sh_blue;

	return irradiance_sh;
};

#include "SimLumenSurfaceCacheCommon.hlsl"

#define RADIOSITY_TRACE_THREADGROUP_SIZE 16
[numthreads(RADIOSITY_TRACE_THREADGROUP_SIZE, RADIOSITY_TRACE_THREADGROUP_SIZE, 1)]
void LumenRadiosityIntegrateCS(uint3 thread_index : SV_DispatchThreadID)
{
    uint2 card_idx_2d = thread_index.xy / SURFACE_CACHE_CARD_SIZE;
    uint2 sub_card_idx = thread_index.xy % SURFACE_CACHE_CARD_SIZE;
    uint2 tile_idx = sub_card_idx / SURFACE_CACHE_PROBE_TEXELS_SIZE;
    uint2 probe_start_pos = card_idx_2d * SURFACE_CACHE_CARD_SIZE + tile_idx * SURFACE_CACHE_PROBE_TEXELS_SIZE;
    uint2 sub_tile_pos = thread_index.xy - probe_start_pos;

    uint2 global_tile_idx = thread_index.xy / SURFACE_CACHE_PROBE_TEXELS_SIZE;
    uint2 pixel_atlas_pos = uint2(thread_index.x, SURFACE_CACHE_TEX_SIZE - thread_index.y);
    float depth = scene_card_depth.Load(int3(pixel_atlas_pos.xy,0));

    if(depth != 0)
    {
        float2 bilinear_weight = float2(sub_tile_pos.x, sub_tile_pos.y) / float2(SURFACE_CACHE_PROBE_TEXELS_SIZE, SURFACE_CACHE_PROBE_TEXELS_SIZE);
        float4 weights = float4(
            (1.0 - bilinear_weight.x) * (1.0 - bilinear_weight.y), // 0 0 
            (1.0 - bilinear_weight.x) * (bilinear_weight.y), // 0 1 
            (bilinear_weight.x) * (1.0 - bilinear_weight.y), // 1 0
            (bilinear_weight.x) * (bilinear_weight.y) // 1 1
        );

        uint2 tile_atlas_index = uint2(global_tile_idx.x, SURFACE_CACHE_ATLAS_TILE_NUM - global_tile_idx.y);
        uint2 ProbeCoord00 = tile_atlas_index;
		uint2 ProbeCoord01 = ProbeCoord00 + uint2(0, 1);
		uint2 ProbeCoord10 = ProbeCoord00 + uint2(1, 0);
		uint2 ProbeCoord11 = ProbeCoord00 + uint2(1, 1);

        FTwoBandSHVectorRGB irradiance_sh = (FTwoBandSHVectorRGB)0;

        FTwoBandSHVectorRGB sub_irradiance_sh00 = GetRadiosityProbeSH(ProbeCoord00);
        FTwoBandSHVectorRGB sub_irradiance_sh01 = GetRadiosityProbeSH(ProbeCoord01);
        FTwoBandSHVectorRGB sub_irradiance_sh10 = GetRadiosityProbeSH(ProbeCoord10);
        FTwoBandSHVectorRGB sub_irradiance_sh11 = GetRadiosityProbeSH(ProbeCoord11);

        // todo: check probe is valid
        irradiance_sh = AddSH(irradiance_sh, MulSH(sub_irradiance_sh00, weights.x));
        irradiance_sh = AddSH(irradiance_sh, MulSH(sub_irradiance_sh01, weights.y));
        irradiance_sh = AddSH(irradiance_sh, MulSH(sub_irradiance_sh10, weights.z));
        irradiance_sh = AddSH(irradiance_sh, MulSH(sub_irradiance_sh11, weights.w));

        uint card_index_1d = card_idx_2d.y * SURFACE_CACHE_CARD_NUM_XY + card_idx_2d.x;
        SCardInfo card_info = scene_card_infos[card_index_1d];
        SCardData card_data = GetSurfaceCardData(card_info, float2(uint2(thread_index.xy % 128u)) / 128.0f, pixel_atlas_pos.xy);
        FTwoBandSHVector diffuse_transfer_sh = CalcDiffuseTransferSH(card_data.world_normal, 1.0f);
        
        float3 texel_radiance = max(float3(0.0f, 0.0f, 0.0f), DotSH(irradiance_sh, diffuse_transfer_sh));
        texel_radiance = texel_radiance / (weights.x + weights.y + weights.z + weights.w);
        surface_cache_indirect_lighting[pixel_atlas_pos] = float4(texel_radiance.xyz, 0.0);
    }
}