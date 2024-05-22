#ifndef SIMLUMEN_COMMON
#define SIMLUMEN_COMMON

struct STraceResult
{
    bool is_hit;
    float hit_distance;
    uint hit_mesh_index;
    uint hit_mesh_sdf_card_index;
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
#endif
