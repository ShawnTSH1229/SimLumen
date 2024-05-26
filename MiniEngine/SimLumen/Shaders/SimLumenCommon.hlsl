#ifndef SIMLUMEN_COMMON
#define SIMLUMEN_COMMON

#define GLOBAL_SDF_SIZE_X 96
#define GLOBAL_SDF_SIZE_Y 96
#define GLOBAL_SDF_SIZE_Z 320

#define SCENE_VOXEL_SIZE_X 48
#define SCENE_VOXEL_SIZE_Y 48
#define SCENE_VOXEL_SIZE_Z 160

#define SCENE_SDF_NUM 13

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

static const float3 voxel_light_direction[6] = {
    float3(+1.0,0,0),
    float3(-1.0,0,0),
    
    float3(0,+1.0,0),
    float3(0,-1.0,0),

    float3(0,0,+1.0),
    float3(0,0,-1.0),
};

#endif
