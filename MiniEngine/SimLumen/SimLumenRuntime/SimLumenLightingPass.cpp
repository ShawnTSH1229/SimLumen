#include "SimLumenLightingPass.h"

using namespace Graphics;

void CSimLumenLightingPass::Init()
{
    {
        DXGI_FORMAT ColorFormat = g_SceneColorBuffer.GetFormat();

        m_lighting_sig.Reset(2);
        m_lighting_sig[0].InitAsConstantBuffer(0);
        m_lighting_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4, D3D12_SHADER_VISIBILITY_PIXEL);
        m_lighting_sig.Finalize(L"m_lighting_sig");

        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenLightingPass.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenLightingPass.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        m_lighting_pso = GraphicsPSO(L"m_lighting_pso");
        m_lighting_pso.SetRootSignature(m_lighting_sig);
        m_lighting_pso.SetRasterizerState(RasterizerDefault);
        m_lighting_pso.SetBlendState(BlendDisable);
        m_lighting_pso.SetDepthStencilState(DepthStateDisabled);
        m_lighting_pso.SetInputLayout(0, nullptr);
        m_lighting_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_lighting_pso.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
        m_lighting_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_lighting_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_lighting_pso.Finalize();
    }
}

void CSimLumenLightingPass::Rendering(GraphicsContext& gfxContext)
{
    //GraphicsContext& gfxContext = GraphicsContext::Begin(L"CSimLumenLightingPass");
    gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
    gfxContext.TransitionResource(g_GBufferA, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    gfxContext.TransitionResource(g_GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    gfxContext.ClearColor(g_SceneColorBuffer);
    gfxContext.FlushResourceBarriers();

    gfxContext.SetRootSignature(m_lighting_sig);
    gfxContext.SetPipelineState(m_lighting_pso);
    gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV());
    gfxContext.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());

    gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    gfxContext.SetConstantBuffer(0, GetGlobalResource().m_global_view_constant_buffer);
    gfxContext.SetDynamicDescriptors(1, 0, 1, &g_GBufferA.GetSRV());
    gfxContext.SetDynamicDescriptors(1, 1, 1, &g_GBufferB.GetSRV());
    gfxContext.SetDynamicDescriptors(1, 2, 1, &g_SceneDepthBuffer.GetDepthSRV());
    gfxContext.SetDynamicDescriptors(1, 3, 1, &g_ShadowBuffer.GetDepthSRV());
    gfxContext.Draw(3);
    //gfxContext.Finish();
}
