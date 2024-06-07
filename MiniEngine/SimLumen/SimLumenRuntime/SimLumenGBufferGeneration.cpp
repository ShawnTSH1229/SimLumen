#include "SimLumenGBufferGeneration.h"

using namespace Graphics;

void CSimLumenGBufferGeneration::Init()
{
    {
        m_gbuffer_gen_sig.Reset(4, 0);
        m_gbuffer_gen_sig[0].InitAsConstantBuffer(0);
        m_gbuffer_gen_sig[1].InitAsConstantBuffer(1);
        m_gbuffer_gen_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_gbuffer_gen_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_gbuffer_gen_sig.Finalize(L"m_shadow_rendering_sig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    D3D12_INPUT_ELEMENT_DESC pos_norm_uv[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,      1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    DXGI_FORMAT gbuffer_formats[3] = { g_GBufferA.GetFormat(),g_GBufferB.GetFormat(),g_GBufferC.GetFormat ()};;
    DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();

    // PSO
    {
        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenGBufferGeneration.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenGBufferGeneration.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);
    
        m_gbuffer_gen_pso = GraphicsPSO(L"m_shadow_rendering_pso");
        m_gbuffer_gen_pso.SetRootSignature(m_gbuffer_gen_sig);
        m_gbuffer_gen_pso.SetRasterizerState(RasterizerDefault);
        m_gbuffer_gen_pso.SetBlendState(BlendDisable);
        m_gbuffer_gen_pso.SetDepthStencilState(DepthStateReadWrite);
        m_gbuffer_gen_pso.SetInputLayout(_countof(pos_norm_uv), pos_norm_uv);
        m_gbuffer_gen_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_gbuffer_gen_pso.SetRenderTargetFormats(3, gbuffer_formats, DepthFormat);
        m_gbuffer_gen_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_gbuffer_gen_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_gbuffer_gen_pso.Finalize();
    }
}

void CSimLumenGBufferGeneration::Rendering(GraphicsContext& gfxContext)
{
    //GraphicsContext& gfxContext = GraphicsContext::Begin(L"CSimLumenGBufferGeneration");
    gfxContext.TransitionResource(g_GBufferA, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
    gfxContext.TransitionResource(g_GBufferB, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
    gfxContext.TransitionResource(g_GBufferC, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
    gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, false);
    gfxContext.ClearColor(g_GBufferA);
    gfxContext.ClearColor(g_GBufferB);
    gfxContext.ClearColor(g_GBufferC);
    gfxContext.ClearDepth(g_SceneDepthBuffer);

    D3D12_CPU_DESCRIPTOR_HANDLE g_buffers[3] = { g_GBufferA.GetRTV(),g_GBufferB.GetRTV(),g_GBufferC.GetRTV() };

    gfxContext.SetRenderTargets(3, g_buffers, g_SceneDepthBuffer.GetDSV());
    gfxContext.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());

    gfxContext.SetRootSignature(m_gbuffer_gen_sig);
    gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    gfxContext.SetPipelineState(m_gbuffer_gen_pso);
    gfxContext.SetConstantBuffer(1, GetGlobalResource().m_global_view_constant_buffer);
    gfxContext.FlushResourceBarriers();

    for (int mesh_idx = 0; mesh_idx < GetGlobalResource().m_mesh_instances.size(); mesh_idx++)
    {
        SLumenMeshInstance& lumen_mesh_instance = GetGlobalResource().m_mesh_instances[mesh_idx];

        DynAlloc mesh_consatnt = gfxContext.ReserveUploadMemory(sizeof(SLumenMeshConstant));
        memcpy(mesh_consatnt.DataPtr, &lumen_mesh_instance.m_LumenConstant, sizeof(SLumenMeshConstant));

        gfxContext.SetConstantBuffer(0, mesh_consatnt.GpuAddress);
        gfxContext.SetDynamicDescriptor(2, 0, lumen_mesh_instance.m_tex.GetSRV());
        gfxContext.SetDynamicSampler(3, 0, SamplerLinearClamp);
        gfxContext.SetVertexBuffer(0, lumen_mesh_instance.m_vertex_pos_buffer.VertexBufferView());
        gfxContext.SetVertexBuffer(1, lumen_mesh_instance.m_vertex_norm_buffer.VertexBufferView());
        gfxContext.SetVertexBuffer(2, lumen_mesh_instance.m_vertex_uv_buffer.VertexBufferView());
        gfxContext.SetIndexBuffer(lumen_mesh_instance.m_index_buffer.IndexBufferView());

        gfxContext.DrawIndexed(lumen_mesh_instance.m_mesh_resource.m_indices.size());
    }
    gfxContext.TransitionResource(g_GBufferA, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    gfxContext.TransitionResource(g_GBufferB, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    gfxContext.TransitionResource(g_GBufferC, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    gfxContext.FlushResourceBarriers();
    //gfxContext.Finish();
}
