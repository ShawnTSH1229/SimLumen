#include "SimLumenRadiosity.h"

using namespace Graphics;

void CSimLumenRadiosity::Init()
{
    // surface cache radiosity trace
    {
        m_radiosity_trace_sig.Reset(5, 0);
		m_radiosity_trace_sig[0].InitAsConstantBuffer(0);
		m_radiosity_trace_sig[1].InitAsConstantBuffer(1);
        m_radiosity_trace_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6);
        m_radiosity_trace_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
        m_radiosity_trace_sig[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_radiosity_trace_sig.Finalize(L"m_radiosity_trace_sig");

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenRadiosityTrace.hlsl", L"LumenRadiosityDistanceFieldTracingCS", L"cs_5_1", nullptr, 0);
        m_radiosity_trace_pso.SetRootSignature(m_radiosity_trace_sig);
        m_radiosity_trace_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_radiosity_trace_pso.Finalize();
    }

	// surface cache radiosity filter
	{
		m_radiosity_filter_sig.Reset(2, 0);
		m_radiosity_filter_sig[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		m_radiosity_filter_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		m_radiosity_filter_sig.Finalize(L"m_radiosity_filter_sig");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenRadiosityProbeFilter.hlsl", L"LumenRadiositySpatialFilterProbeRadiance", L"cs_5_1", nullptr, 0);
		m_radiosity_filter_pso.SetRootSignature(m_radiosity_filter_sig);
		m_radiosity_filter_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		m_radiosity_filter_pso.Finalize();
	}

	// surface cache convert to sh
	{
		m_radiosity_convertsh_sig.Reset(3, 0);
		m_radiosity_convertsh_sig[0].InitAsConstantBuffer(0);
		m_radiosity_convertsh_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5);
		m_radiosity_convertsh_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 3);
		m_radiosity_convertsh_sig.Finalize(L"m_radiosity_convertsh_sig");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenRadiosityConvertSH.hlsl", L"LumenRadiosityConvertToSH", L"cs_5_1", nullptr, 0);
		m_radiosity_convertsh_pso.SetRootSignature(m_radiosity_convertsh_sig);
		m_radiosity_convertsh_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		m_radiosity_convertsh_pso.Finalize();
	}

	// surface cache integrate
	{
		m_radiosity_integrate_sig.Reset(3, 0);
		m_radiosity_integrate_sig[0].InitAsConstantBuffer(0);
		m_radiosity_integrate_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 7);
		m_radiosity_integrate_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		m_radiosity_integrate_sig.Finalize(L"m_radiosity_integrate_sig");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenRadiosityIntegrate.hlsl", L"LumenRadiosityIntegrateCS", L"cs_5_1", nullptr, 0);
		m_radiosity_integrate_pso.SetRootSignature(m_radiosity_integrate_sig);
		m_radiosity_integrate_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		m_radiosity_integrate_pso.Finalize();
	}
}

void CSimLumenRadiosity::RadiosityTrace(ComputeContext& cptContext)
{
	//ComputeContext& cptContext = ComputeContext::Begin(L"RadiosityTrace");

	// radiosity trace
	{
		cptContext.SetRootSignature(m_radiosity_trace_sig);
		cptContext.SetPipelineState(m_radiosity_trace_pso);

		cptContext.TransitionResource(GetGlobalResource().m_scene_card_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_atlas_albedo, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_atlas_normal, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_atlas_depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(GetGlobalResource().m_scene_voxel_lighting, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_trace_radiance_atlas, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cptContext.FlushResourceBarriers();

		cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
		cptContext.SetConstantBuffer(1, GetGlobalResource().m_mesh_sdf_brick_tex_info);

		cptContext.SetDynamicDescriptor(2, 0, GetGlobalResource().m_scene_card_infos_gpu.GetSRV());
		cptContext.SetDynamicDescriptor(2, 1, g_atlas_albedo.GetSRV());
		cptContext.SetDynamicDescriptor(2, 2, g_atlas_normal.GetSRV());
		cptContext.SetDynamicDescriptor(2, 3, g_atlas_depth.GetSRV());
		cptContext.SetDynamicDescriptor(2, 4, GetGlobalResource().m_global_sdf_brick_texture.GetSRV());
		cptContext.SetDynamicDescriptor(2, 5, GetGlobalResource().m_scene_voxel_lighting.GetSRV());

		cptContext.SetDynamicSampler(3, 0, SamplerPointClamp);
		cptContext.SetDynamicDescriptor(4, 0, g_trace_radiance_atlas.GetUAV());

		cptContext.Dispatch(2048 / 16, 128 * 5 / 16, 1);
		cptContext.TransitionResource(g_trace_radiance_atlas, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}


	// radiosity filter
	{
		cptContext.SetRootSignature(m_radiosity_filter_sig);
		cptContext.SetPipelineState(m_radiosity_filter_pso);

		cptContext.TransitionResource(g_trace_radiance_atlas, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_atlas_depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_trace_radiance_atlas_filtered, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cptContext.FlushResourceBarriers();

		cptContext.SetDynamicDescriptor(0, 0, g_trace_radiance_atlas.GetSRV());
		cptContext.SetDynamicDescriptor(0, 1, g_atlas_depth.GetSRV());
		cptContext.SetDynamicDescriptor(1, 0, g_trace_radiance_atlas_filtered.GetUAV());

		cptContext.Dispatch(2048 / 16, 128 * 5 / 16, 1);
		cptContext.TransitionResource(g_trace_radiance_atlas_filtered, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// convert to sh
	{
		cptContext.SetRootSignature(m_radiosity_convertsh_sig);
		cptContext.SetPipelineState(m_radiosity_convertsh_pso);

		cptContext.TransitionResource(g_trace_radiance_atlas_filtered, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(GetGlobalResource().m_scene_card_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_atlas_albedo, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_atlas_normal, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_atlas_depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		cptContext.TransitionResource(g_radiosity_probe_sh_red_atlas, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cptContext.TransitionResource(g_radiosity_probe_sh_green_atlas, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cptContext.TransitionResource(g_radiosity_probe_sh_blue_atlas, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		cptContext.FlushResourceBarriers();

		cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);

		cptContext.SetDynamicDescriptor(1, 0, g_trace_radiance_atlas_filtered.GetSRV());
		cptContext.SetDynamicDescriptor(1, 1, GetGlobalResource().m_scene_card_infos_gpu.GetSRV());
		cptContext.SetDynamicDescriptor(1, 2, g_atlas_albedo.GetSRV());
		cptContext.SetDynamicDescriptor(1, 3, g_atlas_normal.GetSRV());
		cptContext.SetDynamicDescriptor(1, 4, g_atlas_depth.GetSRV());

		cptContext.SetDynamicDescriptor(2, 0, g_radiosity_probe_sh_red_atlas.GetUAV());
		cptContext.SetDynamicDescriptor(2, 1, g_radiosity_probe_sh_green_atlas.GetUAV());
		cptContext.SetDynamicDescriptor(2, 2, g_radiosity_probe_sh_blue_atlas.GetUAV());

		cptContext.Dispatch(2048 / 4 / 8, 128 * 5 / 4 / 8, 1);


	}

	// one bounce debug
	static bool integrate_sh = true;
	if (integrate_sh)
	{
		cptContext.SetRootSignature(m_radiosity_integrate_sig);
		cptContext.SetPipelineState(m_radiosity_integrate_pso);
	
		cptContext.TransitionResource(GetGlobalResource().m_scene_card_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
		cptContext.TransitionResource(g_atlas_albedo, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_atlas_normal, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_atlas_depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		cptContext.TransitionResource(g_radiosity_probe_sh_red_atlas, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_radiosity_probe_sh_green_atlas, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.TransitionResource(g_radiosity_probe_sh_blue_atlas, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		cptContext.TransitionResource(g_surface_cache_indirect, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		cptContext.FlushResourceBarriers();
	
		cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
	
		cptContext.SetDynamicDescriptor(1, 0, GetGlobalResource().m_scene_card_infos_gpu.GetSRV());
	
		cptContext.SetDynamicDescriptor(1, 1, g_atlas_albedo.GetSRV());
		cptContext.SetDynamicDescriptor(1, 2, g_atlas_normal.GetSRV());
		cptContext.SetDynamicDescriptor(1, 3, g_atlas_depth.GetSRV());
	
		cptContext.SetDynamicDescriptor(1, 4, g_radiosity_probe_sh_red_atlas.GetSRV());
		cptContext.SetDynamicDescriptor(1, 5, g_radiosity_probe_sh_green_atlas.GetSRV());
		cptContext.SetDynamicDescriptor(1, 6, g_radiosity_probe_sh_blue_atlas.GetSRV());
	
		cptContext.SetDynamicDescriptor(2, 0, g_surface_cache_indirect.GetUAV());
	
		cptContext.Dispatch(2048 / 16, 128 * 5 / 16, 1);
		cptContext.TransitionResource(g_surface_cache_indirect, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		//integrate_sh = false;
	}

	//cptContext.Finish();
}

