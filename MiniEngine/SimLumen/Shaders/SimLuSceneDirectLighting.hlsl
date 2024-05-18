#define CARD_SIZE 128
#define ATLAS_SIZE 4096

#define CARD_INV_SIZE (1.0/128.0)
#define ATLAS_INV_SIZE (1.0/4096.0)

#include "SimLuSceneCommon.hlsl"

RWTexture2D<float4> rw_direct_lighting_atlas : register(u0)

Texture2D<float4> surface_cache_normal : register(t0)
Texture2D<float> surface_cache_depth : register(t1)
StructuredBuffer<SSimLuCard> scene_cards: register(t2);
StructuredBuffer<SSimLuMeshCard> scene_mesh_cards: register(t3);
StructuredBuffer<SSimLuDirectionalLight> scene_lights: register(t4);
StructuredBuffer<uint> cards_to_update_idx: register(t5);

SamplerState g_point_sampler : register(s0);

SSimLuSurfaceCacheData GetSurfaceCacheData_Normal(float2 atlas_texel_uv,SSimLuCard card_data)
{
    SSimLuSurfaceCacheData surface_cache_data;
   
    float depth = surface_cache_depth.Sample(g_point_sampler, atlas_texel_uv).x;
    
    bool is_valid = depth > 0.0f;
    surface_cache_data.valid = is_valid;
    if(is_valid)
    {
        surface_cache_data.albedo = surface_cache_albedo.Sample(g_point_sampler, atlas_texel_uv).x;
        float3 normal = surface_cache_normal.Sample(g_point_sampler, atlas_texel_uv).xyz;
        surface_cache_data.normal = mul(card_data.l2w_rotation, normal);
    }
    return surface_cache_data;
}

[numthreads(CARD_SIZE, CARD_SIZE, 1)]
void SimLuCardDirectLightCS(uint3 group_idx : SV_GroupID, uint3 group_thread_idx : SV_GroupThreadID)
{
    uint card_idx = cards_to_update_idx[group_idx.x];
    uint2 texel_coord_in_tile = group_thread_idx.xy;

    SSimLuCard card_data = scene_cards[card_idx];
    uint2 atlas_texel_index = card_data.atlas_card_index * 128 + texel_coord_in_tile;
    float2 atlas_texel_uv = atlas_texel_index * ATLAS_INV_SIZE;

    SSimLuSurfaceCacheData surface_cache_data = GetSurfaceCacheData_Normal(atlas_texel_uv,card_data);

    rw_direct_lighting_atlas[atlas_texel_index] = float4(0,0,0);
    if(surface_cache_data.valid)
    {
        SSimLuDirectionalLight light = scene_lights[0]; // for test

        float ndotl = saturate(dot(surface_cache_data.world_normal, light.SunDirection));

        //todo: clear the depth cache and set an invalid value
        float3 Irradiance = light.SunIntensity * ndotl;
        rw_direct_lighting_atlas[atlas_texel_index] = float4(Irradiance,1.0);
    }

}