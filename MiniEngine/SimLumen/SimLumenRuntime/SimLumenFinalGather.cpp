#include "SimLumenFinalGather.h"

using namespace Graphics;

void CSimLumenFinalGather::Init()
{
	InitBrdfPdfPso();
    InitLightPdfPso();
    InitStructuredISPso();
}

void CSimLumenFinalGather::InitBrdfPdfPso()
{
    // final gather brdf pdf
    {
        m_brdf_pdf_sig.Reset(4, 0);
        m_brdf_pdf_sig[0].InitAsConstantBuffer(0);
        m_brdf_pdf_sig[1].InitAsConstantBuffer(1);
        m_brdf_pdf_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
        m_brdf_pdf_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_brdf_pdf_sig.Finalize(L"m_brdf_pdf_sig");

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenImportanceSamplingBrdf.hlsl", L"BRDFPdfCS", L"cs_5_1", nullptr, 0);
        m_brdf_pdf_pso.SetRootSignature(m_brdf_pdf_sig);
        m_brdf_pdf_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_brdf_pdf_pso.Finalize();
    }

    // final gather brdf pdf visualization
    {
        m_brdf_pdf_vis_sig.Reset(4, 0);
        m_brdf_pdf_vis_sig[0].InitAsConstantBuffer(0);
        m_brdf_pdf_vis_sig[1].InitAsConstantBuffer(1);
        m_brdf_pdf_vis_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
        m_brdf_pdf_vis_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
        m_brdf_pdf_vis_sig.Finalize(L"m_brdf_pdf_vis_sig");

        D3D_SHADER_MACRO fxc_defines[1] = { D3D_SHADER_MACRO{"ENABLE_VIS_BRDF_PDF","1"} };
        uint32_t defineCount = 1;

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenImportanceSamplingBrdf.hlsl", L"BRDFPdfCS", L"cs_5_1", fxc_defines, defineCount);
        m_brdf_pdf_vis_pso.SetRootSignature(m_brdf_pdf_vis_sig);
        m_brdf_pdf_vis_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_brdf_pdf_vis_pso.Finalize();
    }
}

void CSimLumenFinalGather::InitLightPdfPso()
{
    // final gather lighting pdf
    {
        m_light_pdf_sig.Reset(2, 0);
        m_light_pdf_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
        m_light_pdf_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_light_pdf_sig.Finalize(L"m_light_pdf_sig");

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenImportanceLightingPdf.hlsl", L"LightingPdfCS", L"cs_5_1", nullptr, 0);
        m_light_pdf_pso.SetRootSignature(m_light_pdf_sig);
        m_light_pdf_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_light_pdf_pso.Finalize();
    }
}

void CSimLumenFinalGather::InitStructuredISPso()
{
    m_sturct_is_sig.Reset(3, 0);
    m_sturct_is_sig[0].InitAsConstantBuffer(0);
    m_sturct_is_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
    m_sturct_is_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    m_sturct_is_sig.Finalize(L"m_sturct_is_sig");

    std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenStructuredImportanceSampling.hlsl", L"StructuredImportanceSamplingCS", L"cs_5_1", nullptr, 0);
    m_struct_is_pso.SetRootSignature(m_sturct_is_sig);
    m_struct_is_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
    m_struct_is_pso.Finalize();
}

void CSimLumenFinalGather::Rendering(ComputeContext& cptContext)
{
    BRDFPdfSH(cptContext);
    LightingPdfSH(cptContext);
    BuildStructuredIS(cptContext);
}

void CSimLumenFinalGather::LightingPdfSH(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_light_pdf_sig);
    cptContext.SetPipelineState(m_light_pdf_pso);

    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().g_is_lighting_pdf_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicDescriptor(0, 0, g_GBufferB.GetSRV());
    cptContext.SetDynamicDescriptor(1, 0, GetGlobalResource().g_is_lighting_pdf_buffer.GetUAV());
    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
}

void CSimLumenFinalGather::BRDFPdfSH(ComputeContext& cptContext)
{
    //ComputeContext& cptContext = ComputeContext::Begin(L"SurfaceCacheCombineLighting");

    if (GetGlobalResource().m_visualize_type == 12)
    {
        cptContext.SetRootSignature(m_brdf_pdf_vis_sig);
        cptContext.SetPipelineState(m_brdf_pdf_vis_pso);
    }
    else
    {
        cptContext.SetRootSignature(m_brdf_pdf_sig);
        cptContext.SetPipelineState(m_brdf_pdf_pso);
    }


    cptContext.TransitionResource(g_GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_brdf_pdf_sh, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.TransitionResource(GetGlobalResource().m_brdf_pdf_visualize, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
    cptContext.SetConstantBuffer(1, GetGlobalResource().m_global_view_constant_buffer);
    cptContext.SetDynamicDescriptor(2, 0, g_GBufferB.GetSRV());
    cptContext.SetDynamicDescriptor(2, 1, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(3, 0, GetGlobalResource().m_brdf_pdf_sh.GetUAV());
    if (GetGlobalResource().m_visualize_type == 12)
    {
        cptContext.SetDynamicDescriptor(3, 1, GetGlobalResource().m_brdf_pdf_visualize.GetUAV());
    }

    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
    //cptContext.Finish();
}

void CSimLumenFinalGather::BuildStructuredIS(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_sturct_is_sig);
    cptContext.SetPipelineState(m_struct_is_pso);

    cptContext.TransitionResource(GetGlobalResource().m_brdf_pdf_sh, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().g_is_lighting_pdf_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_struct_is_tex, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
    cptContext.SetDynamicDescriptor(1, 0, GetGlobalResource().m_brdf_pdf_sh.GetSRV());
    cptContext.SetDynamicDescriptor(1, 1, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(1, 2, GetGlobalResource().g_is_lighting_pdf_buffer.GetSRV());
    cptContext.SetDynamicDescriptor(2, 0, GetGlobalResource().m_struct_is_tex.GetUAV());

    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
}
