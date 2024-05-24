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
    float global_sdf_scale_x;

    float3 global_sdf_tex_size_xyz;
    float global_sdf_scale_y;
};


StructuredBuffer<float4x4> instance_buffer : register(t0);
StructuredBuffer<SMeshSDFInfo> scene_sdf_infos : register(t1);

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

Texture3D<float> global_sdf_texture: register(t2);
SamplerState g_global_sdf_sampler : register(s0);
#include "SimLumenGlobalSDFTraceCommon.hlsl"

float4 ps_main(VSOutput vsOutput) : SV_Target0
{
    float3 world_position = vsOutput.world_position;
    float3 ray_direction = normalize(vsOutput.cube_direction);

    float3 visualize_color = float3(0.1,0.1,0.1);
    SGloablSDFHitResult hit_result = (SGloablSDFHitResult)0;
    TraceGlobalSDF(world_position, ray_direction, hit_result);
    if(hit_result.bHit)
    {
        float3 hit_world_position = hit_result.hit_distance * ray_direction + world_position;
        float3 hit_normal = CalculateGloablSDFNormal(hit_world_position);

        if(hit_normal.x > 0.5)
        {
            return float4(1.0,0.0,0.0,1.0);
        }
        else if(hit_normal.x < -0.5)
        {
            return float4(0.2,0.0,0.0,1.0);
        }
        else if(hit_normal.y > 0.5)
        {
            return float4(0.0,1.0,0.0,1.0);
        }
        else if(hit_normal.y < -0.5)
        {
            return float4(0.0,0.2,0.0,1.0);
        }
        else if(hit_normal.z > 0.5)
        {
            return float4(0.0,0.0,1.0,1.0);
        }
        else if(hit_normal.z < -0.5)
        {
            return float4(0.0,0.0,0.2,1.0);
        }
    }
    return float4(visualize_color.xyz, 1.0);
}