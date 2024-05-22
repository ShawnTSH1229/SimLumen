#include "SimLumenCommon.hlsl"

struct VSInput
{
    float3 position : POSITION;
    float3 direction : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 world_position : TEXCOORD0;
    float3 cube_direction : TEXCOORD1;
};


cbuffer SLumenGlobalConstants : register(b0)
{
    float4x4 ViewProjMatrix;
    float3 CameraPos;
    float3 SunDirection;
    float3 SunIntensity;
}

cbuffer CMeshSdfBrickTextureInfo : register(b1)
{
    // scene sdf infomation
    uint2 texture_brick_num_xy;
    float2 sdf_cb_padding0;

    uint3 texture_size_xyz;
    uint scene_mesh_sdf_num;
    
    // global sdf infomation
    float gloabl_sdf_voxel_size;
    float3 gloabl_sdf_center;

    float3 global_sdf_extents;
    float sdf_cb_padding1;
};

StructuredBuffer<float4x4> instance_buffer : register(t0);
StructuredBuffer<SMeshSDFInfo> scene_sdf_infos : register(t1);
Texture3D<float> distance_field_brick_tex: register(t2);
SamplerState g_sampler_point_3d : register(s0);

VSOutput vs_main(VSInput vsInput, uint instanceID : SV_InstanceID)
{
    float4x4 world_matrix = instance_buffer[instanceID];
    float4 position = float4(vsInput.position, 1.0);
    float3 world_position = mul(world_matrix, position).xyz;

    VSOutput vsOutput;
    vsOutput.position = mul(ViewProjMatrix, float4(world_position, 1.0));
    vsOutput.world_position = world_position;
    vsOutput.cube_direction = vsInput.direction;
    return vsOutput;
}

#include "SimLumenSDFTraceCommon.hlsl"

float4 ps_main(VSOutput vsOutput) : SV_Target0
{
    float3 world_position = vsOutput.world_position;
    float3 ray_direction = normalize(vsOutput.cube_direction);

    STraceResult trace_result = (STraceResult)0;
    trace_result.hit_distance = 1000.0f;
    for(uint mesh_idx = 0; mesh_idx < scene_mesh_sdf_num; mesh_idx++)
    {
        RayTraceSingleMeshSDF(world_position, ray_direction, 1000, mesh_idx, trace_result);
    }

    float3 hit_world_position = float3(0,0,0);
    float3 hit_pos_normal = float3(0.1,0.1,0.1);
    if(trace_result.is_hit)
    {
        hit_world_position = world_position + ray_direction * trace_result.hit_distance;
        hit_pos_normal = CalculateMeshSDFWorldNormal(world_position, ray_direction, trace_result);
    }

    return float4(abs(normalize(hit_pos_normal)), 1.0);
}