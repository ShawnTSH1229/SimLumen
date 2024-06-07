#include "SimlumenCommon.hlsl"

void vs_main(
    in uint VertID : SV_VertexID,
    out float2 Tex : TexCoord0,
    out float4 Pos : SV_Position
)
{
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
};

Texture2D<float4> visualize_texture :register(t0);

float4 ps_main(in float2 Tex : TexCoord0, in float4 screen_pos : SV_Position) : SV_Target0
{
    uint2 pix_pos = screen_pos.xy;
    float3 loaded_value = visualize_texture.Load(int3(pix_pos.xy,0)).xyz;
    return float4(loaded_value.xyz,1.0);
}