#include "SimlumenCommon.hlsl"
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
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
    VSOutput vsOutput;

    float4 position = float4(vsInput.position, 1.0);
    float3 normal = vsInput.normal;

    float3 worldPos = mul(WorldMatrix, position).xyz;
    vsOutput.position = mul(ViewProjMatrix, float4(worldPos, 1.0));
    vsOutput.normal = mul((float3x3)WorldMatrix, normal);
    vsOutput.uv = vsInput.uv;
    return vsOutput;
}

struct GBufferOutput
{
    float4 GBufferA: SV_Target0;
    float4 GBufferB: SV_Target1;
};

Texture2D<float4> baseColorTexture          : register(t0);
SamplerState baseColorSampler               : register(s0);

GBufferOutput ps_main(VSOutput vsOutput)
{
    float3 normal = vsOutput.normal;
    float2 tex_uv = vsOutput.uv;
    tex_uv.y = 1.0 - tex_uv.y;

    float3 baseColor = baseColorTexture.Sample(baseColorSampler, tex_uv).xyz * color_multi.xyz;
    GBufferOutput output;
    output.GBufferA = float4(baseColor, 0.5);
    output.GBufferB = float4(normal * 0.5 + 0.5, 0.5);
    return output;
}