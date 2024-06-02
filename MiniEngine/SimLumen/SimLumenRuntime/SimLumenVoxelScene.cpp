#include "SimLumenVoxelScene.h"

using namespace Graphics;
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

	std::shared_ptr<SCompiledShaderCode> p_cs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenVoxelVisibilityBuffer.hlsl", L"SceneVoxelVisibilityUpdate", L"cs_5_1", nullptr, 0);
	m_vox_vis_update_pso.SetRootSignature(m_vox_vis_update_sig);
	m_vox_vis_update_pso.SetComputeShader(p_cs_shader_code->GetBufferPointer(), p_cs_shader_code->GetBufferSize());
	m_vox_vis_update_pso.Finalize();
}

void CSimLumenVoxelScene::UpdateVisibilityBuffer(ComputeContext& cptContext)
{
	//if (need_update_vis_buffer)
	{
		//ComputeContext& cptContext = ComputeContext::Begin(L"UpdateVisibilityBuffer");

		cptContext.SetRootSignature(m_vox_vis_update_sig);
		cptContext.SetPipelineState(m_vox_vis_update_pso);

		cptContext.TransitionResource(GetGlobalResource().scene_voxel_visibility_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		cptContext.TransitionResource(GetGlobalResource().m_scene_sdf_infos_gpu, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cptContext.FlushResourceBarriers();

		cptContext.SetDynamicConstantBufferView(0, sizeof(SLumenSceneInfo), &GetGlobalResource().m_lumen_scene_info);
		cptContext.SetConstantBuffer(1, GetGlobalResource().m_mesh_sdf_brick_tex_info);
		cptContext.SetDynamicDescriptors(2, 0, 1, &GetGlobalResource().scene_voxel_visibility_buffer.GetUAV());
		cptContext.SetDynamicDescriptors(3, 0, 1, &GetGlobalResource().m_scene_sdf_infos_gpu.GetSRV());
		cptContext.SetDynamicDescriptors(4, 0,1,&GetGlobalResource().m_scene_mesh_sdf_brick_texture.GetSRV());
		cptContext.SetDynamicSampler(5, 0, SamplerPointClamp);

		cptContext.Dispatch(SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y * SCENE_VOXEL_SIZE_Z / 64, 6, 1);
		//cptContext.Finish();
		need_update_vis_buffer = false;
	}

}
