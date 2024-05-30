#ifndef SIMLUMEN_COMMON
#define SIMLUMEN_COMMON

#define GLOBAL_SDF_SIZE_X 96
#define GLOBAL_SDF_SIZE_Y 96
#define GLOBAL_SDF_SIZE_Z 320

#define SCENE_VOXEL_SIZE_X 48
#define SCENE_VOXEL_SIZE_Y 48
#define SCENE_VOXEL_SIZE_Z 160

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
    
#define GLOBAL_LUMEN_SCENE_INFO\
    float3 scene_voxel_min_pos;\
    float padding_vox;\
    float3 scene_voxel_max_pos;\
    float voxel_size;\
    uint card_num_xy;\
    uint scene_card_num;\
    uint frame_index;

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

uint GetVoxelIndexFromWorldPos(float3 world_position)
{
    float3 voxel_offset = world_position - scene_voxel_min_pos;
    uint3 voxel_index_3d = voxel_offset / voxel_size;
    uint voxel_index_1d = voxel_index_3d.z * uint(SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y) + voxel_index_3d.y * SCENE_VOXEL_SIZE_X + voxel_index_3d.x;
    return voxel_index_1d;
}

// From Unreal Engine SHCommon.ush:[BEGIN]
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
// From Unreal Engine SHCommon.ush:[END]
#endif
