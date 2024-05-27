#include "SimLumenCommon.hlsl"
struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    uint card_index : TEXCOORD1;
};

cbuffer SceneVoxelVisibilityInfo : register(b0)
{
    GLOBAL_LUMEN_SCENE_INFO
};

cbuffer SLumenGlobalConstants : register(b1)
{
    GLOBAL_VIEW_CONSTANT_BUFFER
};

StructuredBuffer<SCardInfo> scene_card_infos : register(t0);
Texture2D<float4>  visualize_buffer          : register(t1);

VSOutput vs_main(VSInput vsInput, uint instance_idx : SV_InstanceID)
{
    VSOutput vsOutput;

    SCardInfo card_info = scene_card_infos[instance_idx];
    float3 local_position = float3(0,0,0);
    local_position.xy = vsInput.position.xy * card_info.rotated_extents.xy ;
    local_position.z = card_info.rotated_extents.z;

    float3 rotate_back_pos = mul((float3x3)card_info.rotate_back_matrix, local_position);
    rotate_back_pos += card_info.bound_center;

    float3 world_position = mul(card_info.local_to_world, float4(rotate_back_pos,1.0));
    vsOutput.position = mul(ViewProjMatrix, float4(world_position, 1.0));
    vsOutput.uv = vsInput.uv;
    vsOutput.card_index = instance_idx;
    return vsOutput;
}

float4 ps_main(VSOutput vsOutput) : SV_Target0
{
    uint card_idx = vsOutput.card_index;
    uint2 card_index_xy = uint2((card_idx % card_num_xy), (card_idx / card_num_xy));
    uint2 pixel_pos = card_index_xy * 128 + float2(vsOutput.uv * 128);
    pixel_pos.y = SURFACE_CACHE_TEX_SIZE - pixel_pos.y;
    float3 sampled_value = visualize_buffer.Load(int3(pixel_pos,0)).xyz;
    return float4(sampled_value,1.0);
}
