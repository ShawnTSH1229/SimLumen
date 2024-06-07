#include "SimlumenCommon.hlsl"
cbuffer SLumenGlobalConstants : register(b0)
{
    GLOBAL_VIEW_CONSTANT_BUFFER
};

void vs_main(
    in uint VertID : SV_VertexID,
    out float2 Tex : TexCoord0,
    out float4 Pos : SV_Position
)
{
    Tex = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
};

Texture2D<float4> GBufferA              : register(t0);
Texture2D<float4> GBufferB              : register(t1);
Texture2D<float> DepthBuffer            : register(t2);
Texture2D<float> ShadowDepthBuffer      : register(t3);
Texture2D<float3> ScreenRadiance        : register(t4);

struct SurfaceProperties
{
    float3 N;
    float3 V;
    float3 c_diff;
    float3 c_spec;
    float roughness;
    float alpha; // roughness squared
    float alphaSqr; // alpha squared
    float NdotV;
};

struct LightProperties
{
    float3 L;
    float NdotL;
    float LdotH;
    float NdotH;
};

// Numeric constants
static const float3 kDielectricSpecular = float3(0.04, 0.04, 0.04);

float Pow5(float x)
{
    float xSq = x * x;
    return xSq * xSq * x;
}

float3 Fresnel_Shlick(float3 F0, float3 F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

float Fresnel_Shlick(float F0, float F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

float3 Diffuse_Burley(SurfaceProperties Surface, LightProperties Light)
{
    float fd90 = 0.5 + 2.0 * Surface.roughness * Light.LdotH * Light.LdotH;
    return Surface.c_diff * Fresnel_Shlick(1, fd90, Light.NdotL).x * Fresnel_Shlick(1, fd90, Surface.NdotV).x;
}

float Specular_D_GGX(SurfaceProperties Surface, LightProperties Light)
{
    float lower = lerp(1, Surface.alphaSqr, Light.NdotH * Light.NdotH);
    return Surface.alphaSqr / max(1e-6, PI * lower * lower);
}

float G_Schlick_Smith(SurfaceProperties Surface, LightProperties Light)
{
    return 1.0 / max(1e-6, lerp(Surface.NdotV, 1, Surface.alpha * 0.5) * lerp(Light.NdotL, 1, Surface.alpha * 0.5));
}

float G_Shlick_Smith_Hable(SurfaceProperties Surface, LightProperties Light)
{
    return 1.0 / lerp(Light.LdotH * Light.LdotH, 1, Surface.alphaSqr * 0.25);
}

float3 Specular_BRDF(SurfaceProperties Surface, LightProperties Light)
{
    float ND = Specular_D_GGX(Surface, Light);
    float GV = G_Shlick_Smith_Hable(Surface, Light);
    float3 F = Fresnel_Shlick(Surface.c_spec, 1.0, Light.LdotH);
    return ND * GV * F;
}

float3 ShadeDirectionalLight(SurfaceProperties Surface, float3 L, float3 c_light)
{
    LightProperties Light;
    Light.L = L;

    // Half vector
    float3 H = normalize(L + Surface.V);

    // Pre-compute dot products
    Light.NdotL = saturate(dot(Surface.N, L));
    Light.LdotH = saturate(dot(L, H));
    Light.NdotH = saturate(dot(Surface.N, H));

    // Diffuse & specular factors
    float3 diffuse = Diffuse_Burley(Surface, Light);
    float3 specular = Specular_BRDF(Surface, Light);

    // Directional light
    return Light.NdotL * c_light * (diffuse + specular);
}

float4 ps_main(in float2 Tex : TexCoord0, in float4 screen_pos : SV_Position) : SV_Target0
{
    uint2 pix_pos = screen_pos.xy;
    float depth = DepthBuffer.Load(int3(pix_pos.xy,0)).x;
    float3 colorAccum = float3(0,0,0);
    if(depth != 0.0)
    {
        float3 baseColor = GBufferA.Load(int3(pix_pos.xy,0)).xyz;;
        float3 normal = GBufferB.Load(int3(pix_pos.xy,0)).xyz * 2.0 - 1.0;

        //https://www.jianshu.com/p/308eb5373670
        float4 ndc = float4( Tex.xy * 2.0 - 1.0, depth, 1.0f );
        ndc.y = (ndc.y * (-1.0));
	    float4 wp = mul(InverseViewProjMatrix, ndc);
	    float3 world_position =  wp.xyz / wp.w;

        float shadow = 0.0;
        {
            float4 shadow_screen_pos = mul(ShadowViewProjMatrix, float4(world_position,1.0));
            float2 shadow_uv = shadow_screen_pos.xy;
            shadow_uv = shadow_uv * float2(0.5, -0.5) + float2(0.5, 0.5);
            float2 shadow_pixel_pos = shadow_uv.xy * 2048;

            float shadow_depth_value = ShadowDepthBuffer.Load(int3(shadow_pixel_pos.xy,0)).x;;
            shadow = ((shadow_screen_pos.z + 0.0005) < shadow_depth_value ) ? 0.0 :1.0;
        }

        float2 metallicRoughness = float2(0.5,0.5);

        SurfaceProperties Surface;
        Surface.N = normal;
        Surface.V = normalize(CameraPos - world_position);
        Surface.NdotV = saturate(dot(Surface.N, Surface.V));
        Surface.c_diff = baseColor.rgb * (1 - kDielectricSpecular) * (1 - metallicRoughness.x);
        Surface.c_spec = lerp(kDielectricSpecular, baseColor.rgb, metallicRoughness.x);
        Surface.roughness = metallicRoughness.y;
        Surface.alpha = metallicRoughness.y * metallicRoughness.y;
        Surface.alphaSqr = Surface.alpha * Surface.alpha;

        // directional light
        {
            colorAccum += ShadeDirectionalLight(Surface, SunDirection, SunIntensity) * shadow;
        }

        // point light
        {
            float3 point_light_direction = point_light_world_pos - world_position;
            float light_dist = length(point_light_direction);
            float attenuation = saturate((point_light_radius - light_dist) / point_light_radius);
            colorAccum += ShadeDirectionalLight(Surface, normalize(point_light_direction), float3(1,1,1)) * attenuation * attenuation;
        }

        // indirect lighing
        {
            float3 screen_radiance = ScreenRadiance.Load(int3(pix_pos.xy,0)).xyz;
            colorAccum += (Surface.c_diff * screen_radiance);
        }
        
        
    }

    
    return float4(colorAccum, 1.0);
}