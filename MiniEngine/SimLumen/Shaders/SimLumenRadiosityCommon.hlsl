#include "SimLumenCommon.hlsl"

float2 Hammersley16( uint Index, uint NumSamples, uint2 Random )
{
	float E1 = frac( (float)Index / NumSamples + float( Random.x ) * (1.0 / 65536.0) );
	float E2 = float( ( reversebits(Index) >> 16 ) ^ Random.y ) * (1.0 / 65536.0);
	return float2( E1, E2 );
}

uint3 Rand3DPCG16(int3 p)
{
	uint3 v = uint3(p);
	v = v * 1664525u + 1013904223u;
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;
	v.x += v.y*v.z;
	v.y += v.z*v.x;
	v.z += v.x*v.y;
	return v >> 16u;
}

float2 GetProbeTexelCenter(uint IndirectLightingTemporalIndex, uint2 ProbeTileCoord)
{
    uint2 RandomSeed = Rand3DPCG16(int3(ProbeTileCoord, 0)).xy;
	return Hammersley16(IndirectLightingTemporalIndex % MAX_FRAME_ACCUMULATED, MAX_FRAME_ACCUMULATED, RandomSeed);
}

/* 0 - 3 */
uint2 GetProbeJitter(uint IndirectLightingTemporalIndex)
{
	return Hammersley16(IndirectLightingTemporalIndex % MAX_FRAME_ACCUMULATED, MAX_FRAME_ACCUMULATED, 0) * SURFACE_CACHE_PROBE_TEXELS_SIZE;
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

float3x3 GetTangentBasisFrisvad(float3 TangentZ)
{
	float3 TangentX;
	float3 TangentY;

	if (TangentZ.z < -0.9999999f)
	{
		TangentX = float3(0, -1, 0);
		TangentY = float3(-1, 0, 0);
	}
	else
	{
		float A = 1.0f / (1.0f + TangentZ.z);
		float B = -TangentZ.x * TangentZ.y * A;
		TangentX = float3(1.0f - TangentZ.x * TangentZ.x * A, B, -TangentZ.x);
		TangentY = float3(B, 1.0f - TangentZ.y * TangentZ.y * A, -TangentZ.y);
	}

	return float3x3( TangentX, TangentY, TangentZ );
}

void GetRadiosityRay(uint2 tile_index, uint2 sub_tile_pos, float3 world_normal, out float3 world_ray, out float pdf)
{
    uint indirect_lighting_temporal_index = tile_index.y * (SURFACE_CACHE_CARD_SIZE / 4) + tile_index.x + frame_index;
    float2 probe_texel_jitter = GetProbeTexelCenter(indirect_lighting_temporal_index, tile_index);
    float2 probe_uv = (float2(sub_tile_pos) + probe_texel_jitter) / SURFACE_CACHE_PROBE_TEXELS_SIZE;

    float4 ray_sample = CosineSampleHemisphere(probe_uv);
    float3 local_ray_direction = ray_sample.xyz;
    pdf = ray_sample.w;

    float3x3 tangent_basis = GetTangentBasisFrisvad(world_normal);
    world_ray = mul(local_ray_direction, tangent_basis);
    world_ray = normalize(world_ray);
}