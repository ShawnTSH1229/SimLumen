#include "SimLumenVoxelScene.h"

void CSimLumenVoxelScene::Init()
{
	m_vox_vis_update_sig.Reset(6);
	m_vox_vis_update_sig[0].InitAsConstantBuffer(0);
	m_vox_vis_update_sig[1].InitAsConstantBuffer(1);
	m_vox_vis_update_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
	m_vox_vis_update_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_vox_vis_update_sig[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	m_vox_vis_update_sig[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
	m_vox_vis_update_sig.Finalize(L"m_vox_vis_update_sig");

	std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenVisibilityBuffer.hlsl", L"SceneVoxelVisibilityUpdate", L"cs_5_1", nullptr, 0);
	m_vox_vis_update_pso.SetRootSignature(m_vox_vis_update_sig);
	m_vox_vis_update_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
	m_vox_vis_update_pso.Finalize();
}

void CSimLumenVoxelScene::UpdateVisibilityBuffer()
{
	ComputeContext& cptContext = ComputeContext::Begin(L"update_vis_buffer");

	cptContext.SetRootSignature(m_vox_vis_update_sig);
	cptContext.SetPipelineState(m_vox_vis_update_pso);
	cptContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GetGlobalResource().s_TextureHeap.GetHeapPointer());
	cptContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GetGlobalResource().s_SamplerHeap.GetHeapPointer());

	cptContext.TransitionResource(GetGlobalResource().scene_voxel_visibility_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cptContext.TransitionResource(GetGlobalResource().m_scene_sdf_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cptContext.FlushResourceBarriers();

	cptContext.SetDynamicConstantBufferView(0, sizeof(SceneVoxelVisibilityInfo), &GetGlobalResource().m_scene_voxel_vis_info);
	cptContext.SetConstantBuffer(1, GetGlobalResource().m_mesh_sdf_brick_tex_info);
	cptContext.SetDynamicDescriptors(2, 0, 1, &GetGlobalResource().scene_voxel_visibility_buffer.GetUAV());
	cptContext.SetDynamicDescriptors(3, 0, 1, &GetGlobalResource().m_scene_sdf_infos_gpu.GetSRV());
	cptContext.SetDescriptorTable(4, GetGlobalResource().s_TextureHeap[GetGlobalResource().m_mesh_sdf_brick_tex_table_idx]);
	cptContext.SetDescriptorTable(5, GetGlobalResource().s_SamplerHeap[GetGlobalResource().m_mesh_sdf_brick_tex_sampler_table_idx]);

	cptContext.Dispatch(SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y * SCENE_VOXEL_SIZE_Z / 64, 6, 1);

	cptContext.Finish();
}
