
#include "SimLumenCommon.hlsl"

Texture2D<float3> screen_space_filtered_radiance : register(t0);
RWTexture2D<float3> screen_space_oct_irradiance : register(u0);

#define CONVERT_SH_GROUP_SIZE 8
[numthreads(CONVERT_SH_GROUP_SIZE, CONVERT_SH_GROUP_SIZE, 1)]
void LumenScreenProbeConvertToOCT(uint3 thread_index : SV_DispatchThreadID)
{
    uint2 probe_idx = thread_index.xy;
    uint2 probe_start_pos = probe_idx * SCREEN_SPACE_PROBE;

    FThreeBandSHVectorRGB irradiance_sh = (FThreeBandSHVectorRGB)0;
    for(uint texel_idx_x = 0; texel_idx_x < SCREEN_SPACE_PROBE; texel_idx_x++)
    {
        for(uint texel_idx_y = 0; texel_idx_y < SCREEN_SPACE_PROBE; texel_idx_y++)
        {
            uint2 texel_screen_pos = probe_start_pos + uint2(texel_idx_x, texel_idx_y);

            float2 probe_texel_coord = float2(texel_idx_x, texel_idx_y);
            float2 probe_texel_center = float2(0.5, 0.5);
			float2 probe_uv = (probe_texel_coord + probe_texel_center) / (float)SCREEN_SPACE_PROBE;
			float3 world_cone_direction = EquiAreaSphericalMapping(probe_uv);

            float3 radiance = screen_space_filtered_radiance.Load(int3(texel_screen_pos.xy,0));
            irradiance_sh = AddSH(irradiance_sh, MulSH3(SHBasisFunction3(world_cone_direction), radiance));
        }
    }

    float normalize_weight = 1.0f / float(SCREEN_SPACE_PROBE * SCREEN_SPACE_PROBE);

    irradiance_sh.R = MulSH3(irradiance_sh.R, normalize_weight);
    irradiance_sh.G = MulSH3(irradiance_sh.G, normalize_weight);
    irradiance_sh.B = MulSH3(irradiance_sh.B, normalize_weight);

    for(uint write_idx_x = 0; write_idx_x < SCREEN_SPACE_PROBE; write_idx_x++)
    {
        for(uint write_idx_y = 0; write_idx_y < SCREEN_SPACE_PROBE; write_idx_y++)
        {
            uint2 texel_screen_pos = probe_start_pos + uint2(write_idx_x, write_idx_y);

            uint2 texel_coord_with_boarder = OctahedralMapWrapBorder(uint2(write_idx_x, write_idx_y),SCREEN_SPACE_PROBE,1);
            float2 probe_texel_center = float2(0.5, 0.5);
			float2 probe_uv = (texel_coord_with_boarder + probe_texel_center) / (float)SCREEN_SPACE_PROBE;
            float3 texel_direction = EquiAreaSphericalMapping(probe_uv);

            FThreeBandSHVector diffuse_transfer = CalcDiffuseTransferSH3(texel_direction, 1.0f);
            float3 irradiance = 4.0f * PI * DotSH3(irradiance_sh, diffuse_transfer);
            screen_space_oct_irradiance[texel_screen_pos] = irradiance;
        }
    }
}