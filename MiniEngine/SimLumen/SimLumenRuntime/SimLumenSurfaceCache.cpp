#include "SimLumenSurfaceCache.h"

using namespace Graphics;
void CSimLumenSurfaceCache::Init()
{
    // surface cache direct lighting
    {
        m_surface_cache_direct_light_sig.Reset(4, 0);
        m_surface_cache_direct_light_sig[0].InitAsConstantBuffer(0);
        m_surface_cache_direct_light_sig[1].InitAsConstantBuffer(1);
        m_surface_cache_direct_light_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5);
        m_surface_cache_direct_light_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_surface_cache_direct_light_sig.Finalize(L"m_surface_cache_direct_light_sig");

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenSceneDirectLighting.hlsl", L"LumenCardBatchDirectLightingCS", L"cs_5_1", nullptr, 0);
        m_surface_cache_direct_light_pso.SetRootSignature(m_surface_cache_direct_light_sig);
        m_surface_cache_direct_light_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_surface_cache_direct_light_pso.Finalize();
    }

	// surface cache combine lighting
    {
        m_scache_combine_light_sig.Reset(3, 0);
        m_scache_combine_light_sig[0].InitAsConstantBuffer(0);
        m_scache_combine_light_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3);
        m_scache_combine_light_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
        m_scache_combine_light_sig.Finalize(L"m_scache_combine_light_sig");

        std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenSceneCombineLighting.hlsl", L"LumenSceneCombineLightingCS", L"cs_5_1", nullptr, 0);
        m_scache_combine_light_pso.SetRootSignature(m_scache_combine_light_sig);
        m_scache_combine_light_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
        m_scache_combine_light_pso.Finalize();
    }

	// sruface cache inject lighting
	{
		m_scache_inject_light_sig.Reset(3, 0);
		m_scache_inject_light_sig[0].InitAsConstantBuffer(0);
		m_scache_inject_light_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4);
		m_scache_inject_light_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
		m_scache_inject_light_sig.Finalize(L"m_scache_inject_light_sig");

		std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenLightInjection.hlsl", L"LumenSceneLightInject", L"cs_5_1", nullptr, 0);
		m_scache_inject_light_pso.SetRootSignature(m_scache_inject_light_sig);
		m_scache_inject_light_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
		m_scache_inject_light_pso.Finalize();
	}
}

void CSimLumenSurfaceCache::SurfaceCacheDirectLighting(ComputeContext& cptContext)
{
	//ComputeContext& cptContext = ComputeContext::Begin(L"CSimLumenSurfaceCache");

	cptContext.SetRootSignature(m_surface_cache_direct_light_sig);
	cptContext.SetPipelineState(m_surface_cache_direct_light_pso);

	cptContext.TransitionResource(GetGlobalResource().m_scene_card_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_atlas_albedo, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_atlas_normal, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_atlas_depth, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_ShadowBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_surface_cache_direct, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.FlushResourceBarriers();

	cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
	cptContext.SetConstantBuffer(1, GetGlobalResource().m_global_view_constant_buffer);
	cptContext.SetDynamicDescriptor(2, 0, GetGlobalResource().m_scene_card_infos_gpu.GetSRV());
	cptContext.SetDynamicDescriptor(2, 1, g_atlas_albedo.GetSRV());
	cptContext.SetDynamicDescriptor(2, 2, g_atlas_normal.GetSRV());
	cptContext.SetDynamicDescriptor(2, 3, g_atlas_depth.GetSRV());
	cptContext.SetDynamicDescriptor(2, 4, g_ShadowBuffer.GetSRV());
	cptContext.SetDynamicDescriptor(3, 0, g_surface_cache_direct.GetUAV());

	cptContext.Dispatch(GetGlobalResource().m_atlas_size.x / 8, GetGlobalResource().m_atlas_size.y / 8, 1);
	//cptContext.Finish();
}

void CSimLumenSurfaceCache::SurfaceCacheCombineLighting(ComputeContext& cptContext)
{
	//ComputeContext& cptContext = ComputeContext::Begin(L"SurfaceCacheCombineLighting");

	cptContext.SetRootSignature(m_scache_combine_light_sig);
	cptContext.SetPipelineState(m_scache_combine_light_pso);

	cptContext.TransitionResource(g_surface_cache_direct, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_surface_cache_indirect, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_atlas_albedo, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_surface_cache_final, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.FlushResourceBarriers();

	cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
	cptContext.SetDynamicDescriptor(1, 0, g_surface_cache_direct.GetSRV());
	cptContext.SetDynamicDescriptor(1, 1, g_surface_cache_indirect.GetSRV());
	cptContext.SetDynamicDescriptor(1, 2, g_atlas_albedo.GetSRV());
	cptContext.SetDynamicDescriptor(2, 0, g_surface_cache_final.GetUAV());

	cptContext.Dispatch(GetGlobalResource().m_atlas_size.x / 8, GetGlobalResource().m_atlas_size.y / 8, 1);
	cptContext.TransitionResource(g_surface_cache_final, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//cptContext.Finish();
}

void CSimLumenSurfaceCache::SurfaceCacheInjectLighting(ComputeContext& cptContext)
{
	//ComputeContext& cptContext = ComputeContext::Begin(L"SurfaceCacheInjectLighting");

	cptContext.SetRootSignature(m_scache_inject_light_sig);
	cptContext.SetPipelineState(m_scache_inject_light_pso);

	cptContext.TransitionResource(GetGlobalResource().scene_voxel_visibility_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(GetGlobalResource().m_scene_sdf_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(GetGlobalResource().m_scene_card_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(g_surface_cache_final, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cptContext.TransitionResource(GetGlobalResource().m_scene_voxel_lighting, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.FlushResourceBarriers();

	cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
	cptContext.SetDynamicDescriptor(1, 0, GetGlobalResource().scene_voxel_visibility_buffer.GetSRV());
	cptContext.SetDynamicDescriptor(1, 1, GetGlobalResource().m_scene_sdf_infos_gpu.GetSRV());
	cptContext.SetDynamicDescriptor(1, 2, GetGlobalResource().m_scene_card_infos_gpu.GetSRV());
	cptContext.SetDynamicDescriptor(1, 3, g_surface_cache_final.GetSRV());
	cptContext.SetDynamicDescriptor(2, 0, GetGlobalResource().m_scene_voxel_lighting.GetUAV());

	cptContext.Dispatch(SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y * SCENE_VOXEL_SIZE_Z / 64, 6, 1);
	cptContext.TransitionResource(GetGlobalResource().m_scene_voxel_lighting, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//cptContext.Finish();
}
