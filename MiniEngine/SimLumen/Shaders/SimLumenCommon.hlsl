#ifndef SIMLUMEN_COMMON
#define SIMLUMEN_COMMON

#define GLOBAL_SDF_SIZE_X 96
#define GLOBAL_SDF_SIZE_Y 96
#define GLOBAL_SDF_SIZE_Z 320

#define SCENE_VOXEL_SIZE_X 48
#define SCENE_VOXEL_SIZE_Y 48
#define SCENE_VOXEL_SIZE_Z 160

#define MAX_FRAME_ACCUMULATED 4

#define SCREEN_SPACE_PROBE 8

#define SCENE_SDF_NUM 13
#define SURFACE_CACHE_TEX_SIZE 2048
#define SURFACE_CACHE_CARD_SIZE 128
#define SURFACE_CACHE_CARD_NUM_XY (SURFACE_CACHE_TEX_SIZE / SURFACE_CACHE_CARD_SIZE)
#define SURFACE_CACHE_PROBE_TEXELS_SIZE 4
#define SURFACE_CACHE_CARD_TILE_NUM_XY (SURFACE_CACHE_CARD_SIZE / SURFACE_CACHE_PROBE_TEXELS_SIZE)
#define SURFACE_CACHE_ATLAS_TILE_NUM (SURFACE_CACHE_TEX_SIZE / SURFACE_CACHE_PROBE_TEXELS_SIZE)
#define PI 3.141592653589793284

#define GLOBAL_VIEW_CONSTANT_BUFFER\
    float4x4 ViewProjMatrix;\
    float3 CameraPos;\
    float view_padding0;\
    float3 SunDirection;\
    float view_padding1;\
    float3 SunIntensity;\
    float view_padding2;\
    float4x4 ShadowViewProjMatrix;\
    float4x4 InverseViewProjMatrix;\
    float3 point_light_world_pos;\
    float point_light_radius;\
	float4x4 PreViewProjMatrix;\
	uint visualize_type;\
    
#define GLOBAL_LUMEN_SCENE_INFO\
    float3 scene_voxel_min_pos;\
    float padding_vox;\
    float3 scene_voxel_max_pos;\
    float voxel_size;\
    uint card_num_xy;\
    uint scene_card_num;\
    uint frame_index;\
    uint padding_lumen_info;\
    uint is_pdf_thread_size_x;\
    uint is_pdf_thread_size_y;\
    uint screen_probe_size_x;\
    uint screen_probe_size_y;\

#define GLOBAL_SDF_BUFFER_MEMBER\
    uint2 texture_brick_num_xy;\
    float2 sdf_cb_padding0;\
    uint3 texture_size_xyz;\
    uint scene_mesh_sdf_num;\
    float gloabl_sdf_voxel_size;\
    float3 gloabl_sdf_center;\
    float3 global_sdf_extents;\
    float global_sdf_scale_x;\
    float3 global_sdf_tex_size_xyz;\
    float global_sdf_scale_y;\


struct STraceResult
{
    bool is_hit;
    float hit_distance;
    uint hit_mesh_index;
};

struct SGloablSDFHitResult
{
    bool bHit;
    float hit_distance;
};

struct SMeshSDFInfo
{
    float4x4 volume_to_world;
    float4x4 world_to_volume;

    float3 volume_position_center; // volume space
    float3 volume_position_extent; // volume space

    uint3 volume_brick_num_xyz;

    float volume_brick_size;
    uint volume_brick_start_idx;

    float2 sdf_distance_scale; // x : 2 * max distance , y : - max distance

    uint mesh_card_start_index;

    float padding0;
    float padding1;
};

struct SVoxelDirVisInfo
{
    int mesh_index; // -1: invalid direction
    float hit_distance;
};

struct SVoxelVisibilityInfo
{
    SVoxelDirVisInfo voxel_vis_info[6];
};

struct SCardInfo
{
    float4x4 local_to_world;
    float4x4 world_to_local;
    float4x4 rotate_back_matrix;
    float4x4 rotate_matrix;
    float3 rotated_extents;
    float padding0;
    float3 bound_center;
    uint mesh_index;
};

struct SCardData
{
    bool is_valid;
    float3 world_normal;
    float3 world_position;
    float3 albedo;
};

struct SVoxelLighting
{
    float3 final_lighting[6];
};

static const float3 voxel_light_direction[6] = {
    float3(0,0,-1.0),
    float3(0,0,+1.0),

    float3(0,-1.0,0),
    float3(0,+1.0,0),

    float3(-1.0,0,0),
    float3(+1.0,0,0),
};

float2 GetCardUVFromWorldPos(SCardInfo card_info, float3 world_pos)
{
    float3 local_pos = mul(card_info.world_to_local, float4(world_pos, 1.0)).xyz;
    local_pos = local_pos - card_info.bound_center;
    float3 card_direction_pos = mul((float3x3)card_info.rotate_matrix, local_pos);

    float2 uv = (card_direction_pos.xy / card_info.rotated_extents.xy) * 0.5f + 0.5f;
    return uv;
}

float Luminance(float3 color)
{
	return dot(color, float3(0.3,0.59,0.11));
}


// Copy From Unreal Engine SHCommon.ush:[BEGIN]
struct FOneBandSHVector
{
	half V;
};

struct FOneBandSHVectorRGB
{
	FOneBandSHVector R;
	FOneBandSHVector G;
	FOneBandSHVector B;
};

struct FTwoBandSHVector
{
	half4 V;
};

struct FTwoBandSHVectorRGB
{
	FTwoBandSHVector R;
	FTwoBandSHVector G;
	FTwoBandSHVector B;
};

struct FThreeBandSHVector
{
	half4 V0; // 1 + 3
	half4 V1; // 5
	half V2;
};

struct FThreeBandSHVectorRGB
{
	FThreeBandSHVector R;
	FThreeBandSHVector G;
	FThreeBandSHVector B;
};

FTwoBandSHVectorRGB MulSH(FTwoBandSHVectorRGB A, half Scalar)
{
	FTwoBandSHVectorRGB Result;
	Result.R.V = A.R.V * Scalar;
	Result.G.V = A.G.V * Scalar;
	Result.B.V = A.B.V * Scalar;
	return Result;
}

FTwoBandSHVectorRGB MulSH(FTwoBandSHVector A, half3 Color)
{
	FTwoBandSHVectorRGB Result;
	Result.R.V = A.V * Color.r;
	Result.G.V = A.V * Color.g;
	Result.B.V = A.V * Color.b;
	return Result;
}

FTwoBandSHVector AddSH(FTwoBandSHVector A, FTwoBandSHVector B)
{
	FTwoBandSHVector Result = A;
	Result.V += B.V;
	return Result;
}

FTwoBandSHVectorRGB AddSH(FTwoBandSHVectorRGB A, FTwoBandSHVectorRGB B)
{
	FTwoBandSHVectorRGB Result;
	Result.R = AddSH(A.R, B.R);
	Result.G = AddSH(A.G, B.G);
	Result.B = AddSH(A.B, B.B);
	return Result;
}

FTwoBandSHVector SHBasisFunction(half3 InputVector)
{
	FTwoBandSHVector Result;
	// These are derived from simplifying SHBasisFunction in C++
	Result.V.x = 0.282095f; 
	Result.V.y = -0.488603f * InputVector.y;
	Result.V.z = 0.488603f * InputVector.z;
	Result.V.w = -0.488603f * InputVector.x;
	return Result;
}

FTwoBandSHVector CalcDiffuseTransferSH(half3 Normal,half Exponent)
{
	FTwoBandSHVector Result = SHBasisFunction(Normal);

	half L0 =					2 * PI / (1 + 1 * Exponent							);
	half L1 =					2 * PI / (2 + 1 * Exponent							);

	Result.V.x *= L0;
	Result.V.yzw *= L1;

	return Result;
}

half DotSH(FTwoBandSHVector A,FTwoBandSHVector B)
{
	half Result = dot(A.V, B.V);
	return Result;
}

half3 DotSH(FTwoBandSHVectorRGB A,FTwoBandSHVector B)
{
	half3 Result = 0;
	Result.r = DotSH(A.R,B);
	Result.g = DotSH(A.G,B);
	Result.b = DotSH(A.B,B);
	return Result;
}

FThreeBandSHVector SHBasisFunction3(half3 InputVector)
{
	FThreeBandSHVector Result;
	// These are derived from simplifying SHBasisFunction in C++
	Result.V0.x = 0.282095f; 
	Result.V0.y = -0.488603f * InputVector.y;
	Result.V0.z = 0.488603f * InputVector.z;
	Result.V0.w = -0.488603f * InputVector.x;

	half3 VectorSquared = InputVector * InputVector;
	Result.V1.x = 1.092548f * InputVector.x * InputVector.y;
	Result.V1.y = -1.092548f * InputVector.y * InputVector.z;
	Result.V1.z = 0.315392f * (3.0f * VectorSquared.z - 1.0f);
	Result.V1.w = -1.092548f * InputVector.x * InputVector.z;
	Result.V2 = 0.546274f * (VectorSquared.x - VectorSquared.y);

	return Result;
}


FThreeBandSHVector CalcDiffuseTransferSH3(half3 Normal,half Exponent)
{
	FThreeBandSHVector Result = SHBasisFunction3(Normal);

	// These formula are scaling factors for each SH band that convolve a SH with the circularly symmetric function
	// max(0,cos(theta))^Exponent
	half L0 =					2 * PI / (1 + 1 * Exponent						);
	half L1 =					2 * PI / (2 + 1 * Exponent						);
	half L2 = Exponent *		2 * PI / (3 + 4 * Exponent + Exponent * Exponent);
	half L3 = (Exponent - 1) *	2 * PI / (8 + 6 * Exponent + Exponent * Exponent);

	// Multiply the coefficients in each band with the appropriate band scaling factor.
	Result.V0.x *= L0;
	Result.V0.yzw *= L1;
	Result.V1.xyzw *= L2;
	Result.V2 *= L2;
	return Result;
}

half DotSH3(FThreeBandSHVector A,FThreeBandSHVector B)
{
	half Result = dot(A.V0, B.V0);
	Result += dot(A.V1, B.V1);
	Result += A.V2 * B.V2;
	return Result;
}

half3 DotSH3(FThreeBandSHVectorRGB A,FThreeBandSHVector B)
{
	half3 Result = 0;
	Result.r = DotSH3(A.R,B);
	Result.g = DotSH3(A.G,B);
	Result.b = DotSH3(A.B,B);
	return Result;
}

FThreeBandSHVector AddSH(FThreeBandSHVector A, FThreeBandSHVector B)
{
	FThreeBandSHVector Result = A;
	Result.V0 += B.V0;
	Result.V1 += B.V1;
	Result.V2 += B.V2;
	return Result;
}

FThreeBandSHVectorRGB MulSH3(FThreeBandSHVector A, half3 Color)
{
	FThreeBandSHVectorRGB Result;
	Result.R.V0 = A.V0 * Color.r;
	Result.R.V1 = A.V1 * Color.r;
	Result.R.V2 = A.V2 * Color.r;
	Result.G.V0 = A.V0 * Color.g;
	Result.G.V1 = A.V1 * Color.g;
	Result.G.V2 = A.V2 * Color.g;
	Result.B.V0 = A.V0 * Color.b;
	Result.B.V1 = A.V1 * Color.b;
	Result.B.V2 = A.V2 * Color.b;
	return Result;
}

FThreeBandSHVectorRGB AddSH(FThreeBandSHVectorRGB A, FThreeBandSHVectorRGB B)
{
	FThreeBandSHVectorRGB Result;
	Result.R = AddSH(A.R, B.R);
	Result.G = AddSH(A.G, B.G);
	Result.B = AddSH(A.B, B.B);
	return Result;
}

FThreeBandSHVector MulSH3(FThreeBandSHVector A, half Scalar)
{
	FThreeBandSHVector Result;
	Result.V0 = A.V0 * Scalar;
	Result.V1 = A.V1 * Scalar;
	Result.V2 = A.V2 * Scalar;
	return Result;
}

// Copy From Unreal Engine SHCommon.ush:[END]

// Copy From Unreal Engine MonteCarlo.ush:[BEGIN]
// Based on: [Clarberg 2008, "Fast Equal-Area Mapping of the (Hemi)Sphere using SIMD"]
// Fixed sign bit for UV.y == 0 and removed branch before division by using a small epsilon
// https://fileadmin.cs.lth.se/graphics/research/papers/2008/simdmapping/clarberg_simdmapping08_preprint.pdf
float3 EquiAreaSphericalMapping(float2 UV)
{
	UV = 2 * UV - 1;
	float D = 1 - (abs(UV.x) + abs(UV.y));
	float R = 1 - abs(D);
	// Branch to avoid dividing by 0.
	// Only happens with (0.5, 0.5), usually occurs in odd number resolutions which use the very central texel
	float Phi = R == 0 ? 0 : (PI / 4) * ((abs(UV.y) - abs(UV.x)) / R + 1);
	float F = R * sqrt(2 - R * R);
	return float3(
		F * sign(UV.x) * abs(cos(Phi)),
		F * sign(UV.y) * abs(sin(Phi)),
		sign(D) * (1 - R * R)
	);
}
// Copy From Unreal Engine MonteCarlo.ush:[END]

// Copy From Unreal Engine:[BEGIN]
uint PackRayInfo(uint2 TexelCoord, uint Level)
{
	// Pack in 16 bits
	return (TexelCoord.x & 0x3F) | ((TexelCoord.y & 0x3F) << 6) | ((Level & 0xF) << 12);
}

void UnpackRayInfo(uint RayInfo, out uint2 TexelCoord, out uint Level)
{
	TexelCoord.x = RayInfo & 0x3F;
	TexelCoord.y = (RayInfo >> 6) & 0x3F;
	Level = (RayInfo >> 12) & 0xF;
}

// Wrap around octahedral map for correct hardware bilinear filtering
uint2 OctahedralMapWrapBorder(uint2 TexelCoord, uint Resolution, uint BorderSize)
{
	if (TexelCoord.x < BorderSize)
	{
		TexelCoord.x = BorderSize - 1 + BorderSize - TexelCoord.x;
		TexelCoord.y = Resolution - 1 - TexelCoord.y;
	}
	if (TexelCoord.x >= Resolution - BorderSize)
	{
		TexelCoord.x = (Resolution - BorderSize) - (TexelCoord.x - (Resolution - BorderSize - 1));
		TexelCoord.y = Resolution - 1 - TexelCoord.y;
	}
	if (TexelCoord.y < BorderSize)
	{
		TexelCoord.y = BorderSize - 1 + BorderSize - TexelCoord.y;
		TexelCoord.x = Resolution - 1 - TexelCoord.x;
	}
	if (TexelCoord.y >= Resolution - BorderSize)
	{
		TexelCoord.y = (Resolution - BorderSize) - (TexelCoord.y - (Resolution - BorderSize - 1));
		TexelCoord.x = Resolution - 1 - TexelCoord.x;
	}

	return TexelCoord - BorderSize;
}

// Based on: [Clarberg 2008, "Fast Equal-Area Mapping of the (Hemi)Sphere using SIMD"]
// Removed branch before division by using a small epsilon
// https://fileadmin.cs.lth.se/graphics/research/papers/2008/simdmapping/clarberg_simdmapping08_preprint.pdf
float2 InverseEquiAreaSphericalMapping(float3 Direction)
{
	// Most use cases of this func generate Direction by diffing two positions and thus unnormalized
	Direction = normalize(Direction);
	
	float3 AbsDir = abs(Direction);
	float R = sqrt(1 - AbsDir.z);
	float Epsilon = 5.42101086243e-20; // 2^-64 (this avoids 0/0 without changing the rest of the mapping)
	float x = min(AbsDir.x, AbsDir.y) / (max(AbsDir.x, AbsDir.y) + Epsilon);

	// Coefficients for 6th degree minimax approximation of atan(x)*2/pi, x=[0,1].
	const float t1 = 0.406758566246788489601959989e-5f;
	const float t2 = 0.636226545274016134946890922156f;
	const float t3 = 0.61572017898280213493197203466e-2f;
	const float t4 = -0.247333733281268944196501420480f;
	const float t5 = 0.881770664775316294736387951347e-1f;
	const float t6 = 0.419038818029165735901852432784e-1f;
	const float t7 = -0.251390972343483509333252996350e-1f;

	// Polynomial approximation of atan(x)*2/pi
	float Phi = t6 + t7 * x;
	Phi = t5 + Phi * x;
	Phi = t4 + Phi * x;
	Phi = t3 + Phi * x;
	Phi = t2 + Phi * x;
	Phi = t1 + Phi * x;

	Phi = (AbsDir.x < AbsDir.y) ? 1 - Phi : Phi;
	float2 UV = float2(R - Phi * R, Phi * R);
	UV = (Direction.z < 0) ? 1 - UV.yx : UV;
	UV = asfloat(asuint(UV) ^ (asuint(Direction.xy) & 0x80000000u));
	return UV * 0.5 + 0.5;
}
// Copy From Unreal Engine:[END]
#endif
