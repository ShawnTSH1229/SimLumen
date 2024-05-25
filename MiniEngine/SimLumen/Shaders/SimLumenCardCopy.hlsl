cbuffer SSimLuCardCopy : register(b0)
{
    // index [0 - 32)
    // scale 128 / 4096
    float4 dest_atlas_index_and_scale;
};

struct VSInput
{
    float3 position : POSITION;
    float2 uv: TEXCOORD0;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput vs_main(VSInput vs_in)
{
    // SV_Position left-top 0-0 right-bottom 4096-4096
    // 0 - 1
    float2 dest_pos = (dest_atlas_index_and_scale.xy  + vs_in.position.xy ) * dest_atlas_index_and_scale.zw;
    dest_pos = (dest_pos * 2.0 - 1.0);

    // todo:  we should use a fixed camera and rotate the mesh card

    VSOutput vs_out;
    vs_out.pos = float4(dest_pos, 1.0, 1.0);
    vs_out.uv = vs_in.uv;
    return vs_out;
}

Texture2D<float4> albedo_tex          : register(t0);
Texture2D<float4> normal_tex          : register(t1);
Texture2D<float> depth_tex          : register(t2);

SamplerState point_sampler               : register(s0);

struct PSOutput
{
    float3 albedo : SV_Target0;
    float3 normal : SV_Target1;
    float depth : SV_Target2;
};

PSOutput ps_main(VSOutput vs_out)
{
    float2 tex_uv = vs_out.uv;

    PSOutput ps_out;
    ps_out.albedo = albedo_tex.Sample(point_sampler, tex_uv).xyz;
    ps_out.normal = normal_tex.Sample(point_sampler, tex_uv).xyz;
    ps_out.depth = depth_tex.Sample(point_sampler, tex_uv).x;
    return ps_out;
}