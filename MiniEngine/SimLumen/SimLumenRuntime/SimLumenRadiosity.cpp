#include "SimLumenRadiosity.h"

using namespace Graphics;

void CSimLumenRadiosity::Init()
{
    // surface cache radiosity
    {
        m_radiosity_trace_sig.Reset(4, 0);
        m_radiosity_trace_sig[0].InitAsConstantBuffer(0);
        m_radiosity_trace_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6);
        m_radiosity_trace_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
        m_radiosity_trace_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_radiosity_trace_sig.Finalize(L"m_radiosity_trace_sig");

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenRadiosityTrace.hlsl", L"LumenRadiosityDistanceFieldTracingCS", L"cs_5_1", nullptr, 0);
        m_radiosity_trace_pso.SetRootSignature(m_radiosity_trace_sig);
        m_radiosity_trace_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_radiosity_trace_pso.Finalize();
    }
}

void CSimLumenRadiosity::RadiosityTrace()
{
	ComputeContext& cptContext = ComputeContext::Begin(L"RadiosityTrace");

	cptContext.SetRootSignature(m_radiosity_trace_sig);
	cptContext.SetPipelineState(m_radiosity_trace_pso);

	cptContext.TransitionResource(GetGlobalResource().m_scene_card_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_atlas_albedo, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_atlas_normal, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_atlas_depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(GetGlobalResource().m_scene_voxel_lighting, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_trace_radiance_atlas, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.FlushResourceBarriers();

	cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);

	cptContext.SetDynamicDescriptor(1, 0, GetGlobalResource().m_scene_card_infos_gpu.GetSRV());
	cptContext.SetDynamicDescriptor(1, 1, g_atlas_albedo.GetSRV());
	cptContext.SetDynamicDescriptor(1, 2, g_atlas_normal.GetSRV());
	cptContext.SetDynamicDescriptor(1, 3, g_atlas_depth.GetSRV());
	cptContext.SetDynamicDescriptor(1, 4, GetGlobalResource().m_global_sdf_brick_texture.GetSRV());
	cptContext.SetDynamicDescriptor(1, 5, GetGlobalResource().m_scene_voxel_lighting.GetSRV());

	cptContext.SetDynamicSampler(2, 0, SamplerPointClamp);
	cptContext.SetDynamicDescriptor(3, 0, g_trace_radiance_atlas.GetUAV());

	cptContext.Dispatch(512 * 2048 / (16 * 16), 1 ,1);
	cptContext.Finish();
}
