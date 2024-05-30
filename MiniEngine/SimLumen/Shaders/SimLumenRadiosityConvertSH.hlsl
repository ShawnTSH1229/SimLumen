#include "SimLumenCommon.hlsl"

cbuffer CBLumenSceneInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

Texture2D<float4> trace_radiance_atlas : register(t0);
StructuredBuffer<SCardInfo> scene_card_infos : register(t0);

RWTexture2D<float4> radiosity_probe_sh_red_atlas : register(u0);
RWTexture2D<float4> radiosity_probe_sh_blue_atlas : register(u1);
RWTexture2D<float4> radiosity_probe_sh_green_atlas : register(u2);

#include "SimLumenRadiosityCommon.hlsl"
#include "SimLumenSurfaceCacheCommon.hlsl"

void LumenRadiosityConvertToSH(uint3 thread_index : SV_DispatchThreadID)
{
    uint2 tile_idx = thread_index.xy;
    uint2 probe_start_pos = tile_idx * SURFACE_CACHE_PROBE_TEXELS_SIZE;

    uint2 card_index_2d = tile_idx / SURFACE_CACHE_CARD_TILE_NUM_XY;
    uint card_index_1d = card_index_2d.y * SURFACE_CACHE_CARD_NUM_XY + card_index_2d.x;
    SCardInfo card_info = scene_card_infos[card_index_1d];

    FTwoBandSHVectorRGB irradiance_sh = (FTwoBandSHVectorRGB)0;
    uint num_valid_sample = 0;
    
    for(uint trace_idx_x = 0; trace_idx_x < SURFACE_CACHE_PROBE_TEXELS_SIZE; trace_idx_x++)
    {
        for(uint trace_idx_y = 0; trace_idx_y < SURFACE_CACHE_PROBE_TEXELS_SIZE; trace_idx_y++)
        {
            uint2 pixel_logic_pos = probe_start_pos + uint2(trace_idx_x, trace_idx_y);
            uint2 pixel_atlas_pos = (pixel_logic_pos.x, SURFACE_CACHE_TEX_SIZE - pixel_logic_pos.y);

            SCardData card_data = GetSurfaceCardData(card_info, float2(uint2(pixel_logic_pos.xy % 128u)) / 128.0f, pixel_atlas_pos.xy);

            uint2 sub_tile_pos = uint2(trace_idx_x, trace_idx_y);
            float3 world_ray;
            float pdf;
            GetRadiosityRay(tile_idx, sub_tile_pos, card_data.world_normal, world_ray, pdf);

            float3 trace_irradiance = trace_radiance_atlas.Load(int3(pixel_atlas_pos.xy, 0)).xyz;
            irradiance_sh = AddSH(irradiance_sh, MulSH(SHBasisFunction(world_ray), trace_irradiance / pdf));
			num_valid_sample += 1.0f;
        }
    }

    uint2 tile_atlas_index = uint2(tile_idx.x, SURFACE_CACHE_ATLAS_TILE_NUM - tile_idx.y);
    radiosity_probe_sh_red_atlas[tile_atlas_index] = irradiance_sh.R.V;
	radiosity_probe_sh_blue_atlas[tile_atlas_index] = irradiance_sh.G.V;
	radiosity_probe_sh_green_atlas[tile_atlas_index] = irradiance_sh.B.V;
}