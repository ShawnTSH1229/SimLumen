#include "SimlumenCommon.hlsl"
struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

cbuffer SLumenMeshConstants : register(b0)
{
    float4x4 WorldMatrix;   // Object to world
    float3x3 WorldIT;       // Object normal to world normal
    float4 color_multi;
};

cbuffer SLumenGlobalConstants : register(b1)
{
    GLOBAL_VIEW_CONSTANT_BUFFER
}

VSOutput vs_main(VSInput vsInput)
{
    float4 position = float4(vsInput.position, 1.0);
    float3 worldPos = mul(WorldMatrix, position).xyz;

    VSOutput vsOutput;
    vsOutput.position = mul(ShadowViewProjMatrix, float4(worldPos, 1.0));
    return vsOutput;
}

void ps_main(VSOutput vsOutput) 
{
    return;
}