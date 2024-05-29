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
#define SURFACE_CACHE_PROBE_TEXELS_SIZE 4
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
#endif
