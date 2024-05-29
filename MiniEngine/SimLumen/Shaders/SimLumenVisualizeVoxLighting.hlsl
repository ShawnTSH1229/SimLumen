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

StructuredBuffer<float4x4> instance_buffer : register(t0);

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

cbuffer SceneVoxelVisibilityInfo : register(b1)
{
    GLOBAL_LUMEN_SCENE_INFO
};

StructuredBuffer<SVoxelLighting> scene_voxel_lighting: register(t1);

float4 ps_main(VSOutput vsOutput) : SV_Target0
{
    float3 world_position = vsOutput.world_position;
    float3 voxel_offset = world_position - scene_voxel_min_pos;
    float3 final_color = float3(0.1, 0.1, 0.1);
    if(all(world_position > scene_voxel_min_pos) && all(world_position < scene_voxel_max_pos))
    {
        uint3 voxel_index_3d = voxel_offset / voxel_size;
        uint voxel_index_1d = voxel_index_3d.z * uint(SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y) + voxel_index_3d.y * SCENE_VOXEL_SIZE_X + voxel_index_3d.x;
        
        int dir_index = 0;
        float3 norm_dir = normalize(vsOutput.cube_direction);
        if(abs(norm_dir.z) > 0.5)
        {
            if(norm_dir.z < 0.0)
            {
                dir_index = 0;
            }
            else
            {
                dir_index = 1;
            }
        }
        
        if(abs(norm_dir.y) > 0.5)
        {
            if(norm_dir.y < 0.0)
            {
                dir_index = 2;
            }
            else
            {
                dir_index = 3;
            }
        }

        if(abs(norm_dir.x) > 0.5)
        {
            if(norm_dir.x < 0.0)
            {
                dir_index = 4;
            }
            else
            {
                dir_index = 5;
            }
        }

        SVoxelLighting voxel_lighting = scene_voxel_lighting[voxel_index_1d];
        final_color = voxel_lighting.final_lighting[dir_index];
    }
    return float4(final_color, 1.0);
};