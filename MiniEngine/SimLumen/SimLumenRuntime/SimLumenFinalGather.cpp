#include "SimLumenFinalGather.h"

using namespace Graphics;

void CSimLumenFinalGather::Init()
{
	InitBrdfPdfPso();
    InitLightPdfPso();
    InitStructuredISPso();

    InitSSProbeTraceMeshSDFPso();
    InitSSProbeTraceVoxelPso();
    InitSSProbeCompositePso();
    InitSSProbeFilterPso();
    InitSSProbeConvertToOCTPso();
    InitSSProbeIntegratePso();
}

void CSimLumenFinalGather::InitBrdfPdfPso()
{
    // final gather brdf pdf
    {
        m_brdf_pdf_sig.Reset(4, 0);
        m_brdf_pdf_sig[0].InitAsConstantBuffer(0);
        m_brdf_pdf_sig[1].InitAsConstantBuffer(1);
        m_brdf_pdf_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
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
        m_brdf_pdf_vis_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
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

    // final gather lighting pdf
    {
        m_light_hist_pdf_sig.Reset(4, 0);
        m_light_hist_pdf_sig[0].InitAsConstantBuffer(0);
        m_light_hist_pdf_sig[1].InitAsConstantBuffer(1);
        m_light_hist_pdf_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
        m_light_hist_pdf_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_light_hist_pdf_sig.Finalize(L"m_light_hist_pdf_sig");

        D3D_SHADER_MACRO fxc_define[1] = { D3D_SHADER_MACRO{"PROBE_RADIANCE_HISTORY","1"} };

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenImportanceLightingPdf.hlsl", L"LightingPdfCS", L"cs_5_1", fxc_define, 1);
        m_light_hist_pdf_pso.SetRootSignature(m_light_hist_pdf_sig);
        m_light_hist_pdf_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_light_hist_pdf_pso.Finalize();
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

void CSimLumenFinalGather::InitSSProbeTraceMeshSDFPso()
{
    m_ssprobe_trace_mesh_sdf_sig.Reset(6, 0);
    m_ssprobe_trace_mesh_sdf_sig[0].InitAsConstantBuffer(0);
    m_ssprobe_trace_mesh_sdf_sig[1].InitAsConstantBuffer(1);
    m_ssprobe_trace_mesh_sdf_sig[2].InitAsConstantBuffer(2);
    m_ssprobe_trace_mesh_sdf_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 8);
    m_ssprobe_trace_mesh_sdf_sig[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    m_ssprobe_trace_mesh_sdf_sig[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
    m_ssprobe_trace_mesh_sdf_sig.Finalize(L"m_ssprobe_trace_mesh_sdf_sig");

    std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenScreenProbeTraceMeshSDF.hlsl", L"ScreenProbeTraceMeshSDFsCS", L"cs_5_1", nullptr, 0);
    m_ssprobe_trace_mesh_sdf_pso.SetRootSignature(m_ssprobe_trace_mesh_sdf_sig);
    m_ssprobe_trace_mesh_sdf_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
    m_ssprobe_trace_mesh_sdf_pso.Finalize();
}

void CSimLumenFinalGather::InitSSProbeTraceVoxelPso()
{
    m_ssprobe_voxel_sig.Reset(6, 0);
    m_ssprobe_voxel_sig[0].InitAsConstantBuffer(0);
    m_ssprobe_voxel_sig[1].InitAsConstantBuffer(1);
    m_ssprobe_voxel_sig[2].InitAsConstantBuffer(2);
    m_ssprobe_voxel_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6);
    m_ssprobe_voxel_sig[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
    m_ssprobe_voxel_sig[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
    m_ssprobe_voxel_sig.Finalize(L"m_ssprobe_voxel_sig");

    std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenScreenSpaceProbeTraceVoxels.hlsl", L"ScreenProbeTraceVoxelCS", L"cs_5_1", nullptr, 0);
    m_ssprobe_voxel_pso.SetRootSignature(m_ssprobe_voxel_sig);
    m_ssprobe_voxel_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
    m_ssprobe_voxel_pso.Finalize();
}

void CSimLumenFinalGather::InitSSProbeCompositePso()
{
    m_ssprobe_composite_sig.Reset(2, 0);
    m_ssprobe_composite_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
    m_ssprobe_composite_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    m_ssprobe_composite_sig.Finalize(L"m_ssprobe_composite_sig");

    std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenScreenProbeComposite.hlsl", L"ScreenProbeCompositeCS", L"cs_5_1", nullptr, 0);
    m_ssprobe_composite_pso.SetRootSignature(m_ssprobe_composite_sig);
    m_ssprobe_composite_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
    m_ssprobe_composite_pso.Finalize();
}

void CSimLumenFinalGather::InitSSProbeFilterPso()
{
    m_ssprobe_filter_sig.Reset(3, 0);
    m_ssprobe_filter_sig[0].InitAsConstantBuffer(0);
    m_ssprobe_filter_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
    m_ssprobe_filter_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    m_ssprobe_filter_sig.Finalize(L"m_ssprobe_filter_sig");

    std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenScreenProbeFilter.hlsl", L"ScreenProbeFilterCS", L"cs_5_1", nullptr, 0);
    m_ssprobe_filter_pso.SetRootSignature(m_ssprobe_filter_sig);
    m_ssprobe_filter_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
    m_ssprobe_filter_pso.Finalize();
}

void CSimLumenFinalGather::InitSSProbeConvertToOCTPso()
{
    m_ssprobe_to_oct_sig.Reset(2, 0);
    m_ssprobe_to_oct_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
    m_ssprobe_to_oct_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    m_ssprobe_to_oct_sig.Finalize(L"m_ssprobe_to_oct_sig");

    std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenScreenProbeOctIrradiance.hlsl", L"LumenScreenProbeConvertToOCT", L"cs_5_1", nullptr, 0);
    m_ssprobe_to_oct_pso.SetRootSignature(m_ssprobe_to_oct_sig);
    m_ssprobe_to_oct_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
    m_ssprobe_to_oct_pso.Finalize();
}

void CSimLumenFinalGather::InitSSProbeIntegratePso()
{
    m_ssprobe_integrate_sig.Reset(4, 0);
    m_ssprobe_integrate_sig[0].InitAsConstantBuffer(0);
    m_ssprobe_integrate_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
    m_ssprobe_integrate_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    m_ssprobe_integrate_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
    m_ssprobe_integrate_sig.Finalize(L"m_ssprobe_integrate_sig");

    std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenScreenProbeIntegrate.hlsl", L"ScreenProbeIntegrateCS", L"cs_5_1", nullptr, 0);
    m_ssprobe_integrate_pso.SetRootSignature(m_ssprobe_integrate_sig);
    m_ssprobe_integrate_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
    m_ssprobe_integrate_pso.Finalize();
}

void CSimLumenFinalGather::Rendering(ComputeContext& cptContext)
{
    static bool is_first_frame = true;
    // importance sampling
    BRDFPdfSH(cptContext);
    if (is_first_frame || (GetGlobalResource().m_disable_lighd_is == true))
    {
        LightingPdfSH(cptContext);
    }
    else
    {
        LightingHistPdfSH(cptContext);
    }
    is_first_frame = false;
    BuildStructuredIS(cptContext);

    // filter and gather
    SSProbeTraceMeshSDF(cptContext);
    SSProbeTraceVoxel(cptContext);
    SSProbeComposite(cptContext);
    SSProbeFilter(cptContext);
    SSProbeToOct(cptContext);
    SSProbeIntegrate(cptContext);
}

void CSimLumenFinalGather::LightingPdfSH(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_light_pdf_sig);
    cptContext.SetPipelineState(m_light_pdf_pso);


    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().g_is_lighting_pdf_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicDescriptor(0, 0, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(1, 0, GetGlobalResource().g_is_lighting_pdf_buffer.GetUAV());
    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
}

void CSimLumenFinalGather::LightingHistPdfSH(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_light_hist_pdf_sig);
    cptContext.SetPipelineState(m_light_hist_pdf_pso);

    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_GBufferC, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_sspace_radiance, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().g_is_lighting_pdf_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
    cptContext.SetConstantBuffer(1, GetGlobalResource().m_global_view_constant_buffer);
    cptContext.SetDynamicDescriptor(2, 0, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(2, 1, g_GBufferC.GetSRV());
    cptContext.SetDynamicDescriptor(2, 2, GetGlobalResource().m_sspace_radiance.GetSRV());
    cptContext.SetDynamicDescriptor(3, 0, GetGlobalResource().g_is_lighting_pdf_buffer.GetUAV());
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
    cptContext.TransitionResource(g_GBufferC, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
    cptContext.SetConstantBuffer(1, GetGlobalResource().m_global_view_constant_buffer);
    cptContext.SetDynamicDescriptor(2, 0, g_GBufferB.GetSRV());
    cptContext.SetDynamicDescriptor(2, 1, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(2, 2, g_GBufferC.GetSRV());
    cptContext.SetDynamicDescriptor(3, 0, GetGlobalResource().m_brdf_pdf_sh.GetUAV());
    if (GetGlobalResource().m_visualize_type == 12)
    {
        cptContext.SetDynamicDescriptor(3, 1, GetGlobalResource().m_brdf_pdf_visualize.GetUAV());
    }

    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
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

void CSimLumenFinalGather::SSProbeTraceMeshSDF(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_ssprobe_trace_mesh_sdf_sig);
    cptContext.SetPipelineState(m_ssprobe_trace_mesh_sdf_pso);

    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_struct_is_tex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_scene_sdf_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_scene_card_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_surface_cache_final, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_GBufferC, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cptContext.TransitionResource(GetGlobalResource().m_ssprobe_type, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.TransitionResource(GetGlobalResource().m_sspace_trace_radiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
    cptContext.SetConstantBuffer(1, GetGlobalResource().m_global_view_constant_buffer);
    cptContext.SetConstantBuffer(2, GetGlobalResource().m_mesh_sdf_brick_tex_info);

    cptContext.SetDynamicDescriptor(3, 0, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(3, 1, GetGlobalResource().m_struct_is_tex.GetSRV());
    cptContext.SetDynamicDescriptor(3, 2, GetGlobalResource().m_scene_sdf_infos_gpu.GetSRV());
    cptContext.SetDynamicDescriptor(3, 3, GetGlobalResource().m_scene_mesh_sdf_brick_texture.GetSRV());
    cptContext.SetDynamicDescriptor(3, 4, GetGlobalResource().m_scene_card_infos_gpu.GetSRV());
    cptContext.SetDynamicDescriptor(3, 5, g_surface_cache_final.GetSRV());
    cptContext.SetDynamicDescriptor(3, 6, g_GBufferB.GetSRV());
    cptContext.SetDynamicDescriptor(3, 7, g_GBufferC.GetSRV());

    cptContext.SetDynamicDescriptor(4, 0, GetGlobalResource().m_ssprobe_type.GetUAV());
    cptContext.SetDynamicDescriptor(4, 1, GetGlobalResource().m_sspace_trace_radiance.GetUAV());

    cptContext.SetDynamicSampler(5, 0, SamplerPointClamp);

    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
}

void CSimLumenFinalGather::SSProbeTraceVoxel(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_ssprobe_voxel_sig);
    cptContext.SetPipelineState(m_ssprobe_voxel_pso);

    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_struct_is_tex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_scene_voxel_lighting, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_GBufferC, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cptContext.TransitionResource(GetGlobalResource().m_ssprobe_type, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.TransitionResource(GetGlobalResource().m_sspace_trace_radiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
    cptContext.SetConstantBuffer(1, GetGlobalResource().m_global_view_constant_buffer);
    cptContext.SetConstantBuffer(2, GetGlobalResource().m_mesh_sdf_brick_tex_info);

    cptContext.SetDynamicDescriptor(3, 0, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(3, 1, GetGlobalResource().m_struct_is_tex.GetSRV());
    cptContext.SetDynamicDescriptor(3, 2, GetGlobalResource().m_scene_voxel_lighting.GetSRV());
    cptContext.SetDynamicDescriptor(3, 3, GetGlobalResource().m_global_sdf_brick_texture.GetSRV());
    cptContext.SetDynamicDescriptor(3, 4, g_GBufferB.GetSRV());
    cptContext.SetDynamicDescriptor(3, 5, g_GBufferC.GetSRV());

    cptContext.SetDynamicDescriptor(4, 0, GetGlobalResource().m_ssprobe_type.GetUAV());
    cptContext.SetDynamicDescriptor(4, 1, GetGlobalResource().m_sspace_trace_radiance.GetUAV());

    cptContext.SetDynamicSampler(5, 0, SamplerPointClamp);

    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
}

void CSimLumenFinalGather::SSProbeComposite(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_ssprobe_composite_sig);
    cptContext.SetPipelineState(m_ssprobe_composite_pso);

    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_struct_is_tex, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_sspace_trace_radiance, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cptContext.TransitionResource(GetGlobalResource().m_sspace_radiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicDescriptor(0, 0, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(0, 1, GetGlobalResource().m_struct_is_tex.GetSRV());
    cptContext.SetDynamicDescriptor(0, 2, GetGlobalResource().m_sspace_trace_radiance.GetSRV());

    cptContext.SetDynamicDescriptor(1, 0, GetGlobalResource().m_sspace_radiance.GetUAV());

    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
}

void CSimLumenFinalGather::SSProbeFilter(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_ssprobe_filter_sig);
    cptContext.SetPipelineState(m_ssprobe_filter_pso);

    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_sspace_radiance, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cptContext.TransitionResource(GetGlobalResource().m_sspace_radiance_filtered, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);

    cptContext.SetDynamicDescriptor(1, 0, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(1, 1, GetGlobalResource().m_sspace_radiance.GetSRV());

    cptContext.SetDynamicDescriptor(2, 0, GetGlobalResource().m_sspace_radiance_filtered.GetUAV());

    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
}

void CSimLumenFinalGather::SSProbeToOct(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_ssprobe_to_oct_sig);
    cptContext.SetPipelineState(m_ssprobe_to_oct_pso);

    cptContext.TransitionResource(GetGlobalResource().m_sspace_radiance_filtered, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(GetGlobalResource().m_sspace_radiance_oct, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicDescriptor(0, 0, GetGlobalResource().m_sspace_radiance_filtered.GetSRV());
    cptContext.SetDynamicDescriptor(1, 0, GetGlobalResource().m_sspace_radiance_oct.GetUAV());

    cptContext.Dispatch((GetGlobalResource().m_lumen_scene_info.screen_probe_size_x + 7)/8, (GetGlobalResource().m_lumen_scene_info.screen_probe_size_y + 7) / 8, 1);
}

void CSimLumenFinalGather::SSProbeIntegrate(ComputeContext& cptContext)
{
    cptContext.SetRootSignature(m_ssprobe_integrate_sig);
    cptContext.SetPipelineState(m_ssprobe_integrate_pso);

    cptContext.TransitionResource(GetGlobalResource().m_sspace_radiance_oct, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cptContext.TransitionResource(g_GBufferB, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cptContext.TransitionResource(GetGlobalResource().m_sspace_final_radiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cptContext.FlushResourceBarriers();

    cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);

    cptContext.SetDynamicDescriptor(1, 0, GetGlobalResource().m_sspace_radiance_oct.GetSRV());
    cptContext.SetDynamicDescriptor(1, 1, g_SceneDepthBuffer.GetDepthSRV());
    cptContext.SetDynamicDescriptor(1, 2, g_GBufferB.GetSRV());

    cptContext.SetDynamicDescriptor(2, 0, GetGlobalResource().m_sspace_final_radiance.GetUAV());

    cptContext.SetDynamicSampler(3, 0, SamplerLinearClamp);

    cptContext.Dispatch(GetGlobalResource().m_lumen_scene_info.screen_probe_size_x, GetGlobalResource().m_lumen_scene_info.screen_probe_size_y, 1);
}
