#include "SimLumenVisualization.h"

using namespace Graphics;

static void CreateSDFVisBox(std::vector<Math::Vector3>& positions, std::vector<unsigned int>& indices, float x, float y, float z)
{
	float half_x = x / 2;
	float half_y = y / 2;
	float half_z = z / 2;

	positions.resize(4 * 6);
	indices.resize(3 * 2 * 6);

	for (int face_idx = 0; face_idx < 6; face_idx++)
	{
		indices[face_idx * 6u + 0u] = face_idx * 4 + 0;
		indices[face_idx * 6u + 1u] = face_idx * 4 + 1;
		indices[face_idx * 6u + 2u] = face_idx * 4 + 2;
		
		indices[face_idx * 6u + 3u] = face_idx * 4 + 0;
		indices[face_idx * 6u + 4u] = face_idx * 4 + 2;
		indices[face_idx * 6u + 5u] = face_idx * 4 + 3;
	}

	// right plane
	positions[0] = DirectX::XMFLOAT3(half_x, half_y, -half_z);
	positions[1] = DirectX::XMFLOAT3(half_x, half_y, half_z);
	positions[2] = DirectX::XMFLOAT3(half_x, -half_y, half_z);
	positions[3] = DirectX::XMFLOAT3(half_x, -half_y, -half_z);

	// left plane
	positions[4] = DirectX::XMFLOAT3(-half_x, half_y, half_z);
	positions[5] = DirectX::XMFLOAT3(-half_x, half_y, -half_z);
	positions[6] = DirectX::XMFLOAT3(-half_x, -half_y, -half_z);
	positions[7] = DirectX::XMFLOAT3(-half_x, -half_y, half_z);

	// top plane
	positions[8] = DirectX::XMFLOAT3(-half_x, half_y, half_z);
	positions[9] = DirectX::XMFLOAT3(half_x, half_y, half_z);
	positions[10] = DirectX::XMFLOAT3(half_x, half_y, -half_z);
	positions[11] = DirectX::XMFLOAT3(-half_x, half_y, -half_z);

	// bottom plane
	positions[12] = DirectX::XMFLOAT3(-half_x, -half_y, -half_z);
	positions[13] = DirectX::XMFLOAT3(half_x, -half_y, -half_z);
	positions[14] = DirectX::XMFLOAT3(half_x, -half_y, half_z);
	positions[15] = DirectX::XMFLOAT3(-half_x, -half_y, half_z);

	// front plane
	positions[16] = DirectX::XMFLOAT3(-half_x, half_y, -half_z);
	positions[17] = DirectX::XMFLOAT3(half_x, half_y, -half_z);
	positions[18] = DirectX::XMFLOAT3(half_x, -half_y, -half_z);
	positions[19] = DirectX::XMFLOAT3(-half_x, -half_y, -half_z);

	// back plane
	positions[20] = DirectX::XMFLOAT3(half_x, half_y, half_z);
	positions[21] = DirectX::XMFLOAT3(-half_x, half_y, half_z);
	positions[22] = DirectX::XMFLOAT3(-half_x, -half_y, half_z);
	positions[23] = DirectX::XMFLOAT3(half_x, -half_y, half_z);
}

void CSimLumenVisualization::Init()
{
	InitSDFVisBuffer();

	D3D12_INPUT_ELEMENT_DESC pos_norm_uv[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT,    1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	DXGI_FORMAT ColorFormat = g_SceneColorBuffer.GetFormat();
	DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();

	// visualize mesh sdf root signature
	{
		m_vis_sdf_sig.Reset(5, 0);
		m_vis_sdf_sig[0].InitAsConstantBuffer(0);
		m_vis_sdf_sig[1].InitAsConstantBuffer(1);
		m_vis_sdf_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		m_vis_sdf_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);
		m_vis_sdf_sig[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
		m_vis_sdf_sig.Finalize(L"m_vis_sdf_sig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	}

	// visualize mesh sdf pso
	{
		std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenVisualizeMeshSDF.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
		std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenVisualizeMeshSDF.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

		m_vis_sdf_pso = GraphicsPSO(L"m_vis_sdf_pso");
		m_vis_sdf_pso.SetRootSignature(m_vis_sdf_sig);
		m_vis_sdf_pso.SetRasterizerState(RasterizerDefault);
		m_vis_sdf_pso.SetBlendState(BlendDisable);
		m_vis_sdf_pso.SetDepthStencilState(DepthStateReadWrite);
		m_vis_sdf_pso.SetInputLayout(_countof(pos_norm_uv), pos_norm_uv);
		m_vis_sdf_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_vis_sdf_pso.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
		m_vis_sdf_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
		m_vis_sdf_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
		m_vis_sdf_pso.Finalize();
	}

	// visualize global sdf root signature
	{
		m_vis_global_sdf_sig.Reset(5, 0);
		m_vis_global_sdf_sig[0].InitAsConstantBuffer(0);
		m_vis_global_sdf_sig[1].InitAsConstantBuffer(1);
		m_vis_global_sdf_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		m_vis_global_sdf_sig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);
		m_vis_global_sdf_sig[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1);
		m_vis_global_sdf_sig.Finalize(L"m_vis_global_sdf_sig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	}

	// visualize global sdf pso
	{
		std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenVisualizeGloablSDF.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
		std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenVisualizeGloablSDF.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

		m_vis_global_sdf_pso = GraphicsPSO(L"m_vis_global_sdf_pso");
		m_vis_global_sdf_pso.SetRootSignature(m_vis_sdf_sig);
		m_vis_global_sdf_pso.SetRasterizerState(RasterizerDefault);
		m_vis_global_sdf_pso.SetBlendState(BlendDisable);
		m_vis_global_sdf_pso.SetDepthStencilState(DepthStateReadWrite);
		m_vis_global_sdf_pso.SetInputLayout(_countof(pos_norm_uv), pos_norm_uv);
		m_vis_global_sdf_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_vis_global_sdf_pso.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
		m_vis_global_sdf_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
		m_vis_global_sdf_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
		m_vis_global_sdf_pso.Finalize();
	}
}


void CSimLumenVisualization::Render()
{
	GraphicsContext& gfxContext = GraphicsContext::Begin(L"Visualize Pass");
	
	if (GetGlobalResource().m_visualize_type == 1)
	{
		VisualizeMeshSDFs(gfxContext);
	}
	else if (GetGlobalResource().m_visualize_type == 2)
	{
		VisualizeGloablSDFs(gfxContext);
	}
	
	gfxContext.Finish();
}

void CSimLumenVisualization::VisualizeMeshSDFs(GraphicsContext& gfxContext)
{
	gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	gfxContext.FlushResourceBarriers();

	gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());
	gfxContext.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());

	gfxContext.SetRootSignature(m_vis_sdf_sig);
	gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gfxContext.SetPipelineState(m_vis_sdf_pso);

	gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GetGlobalResource().s_TextureHeap.GetHeapPointer());
	gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GetGlobalResource().s_SamplerHeap.GetHeapPointer());

	gfxContext.SetConstantBuffer(0, GetGlobalResource().m_global_view_constant_buffer);
	gfxContext.SetConstantBuffer(1, GetGlobalResource().m_mesh_sdf_brick_tex_info);
	gfxContext.SetDynamicDescriptor(2, 0, m_sdf_instance_buffer.GetSRV());
	gfxContext.SetDynamicDescriptor(2, 1, GetGlobalResource().m_scene_sdf_infos_gpu.GetSRV());
	gfxContext.SetDescriptorTable(3, GetGlobalResource().s_TextureHeap[GetGlobalResource().m_mesh_sdf_brick_tex_table_idx]);
	gfxContext.SetDescriptorTable(4, GetGlobalResource().s_SamplerHeap[GetGlobalResource().m_mesh_sdf_brick_tex_sampler_table_idx]);

	gfxContext.SetVertexBuffer(0, m_sdf_vis_pos_buffer.VertexBufferView());
	gfxContext.SetVertexBuffer(1, m_sdf_vis_direction_buffer.VertexBufferView());
	gfxContext.SetIndexBuffer(m_sdf_vis_index_buffer.IndexBufferView());
	gfxContext.DrawIndexedInstanced(m_index_count_perinstance, m_instance_num, 0, 0, 0);
}

void CSimLumenVisualization::VisualizeGloablSDFs(GraphicsContext& gfxContext)
{
	gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	gfxContext.FlushResourceBarriers();

	gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());
	gfxContext.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());

	gfxContext.SetRootSignature(m_vis_global_sdf_sig);
	gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gfxContext.SetPipelineState(m_vis_global_sdf_pso);

	gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GetGlobalResource().s_TextureHeap.GetHeapPointer());
	gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GetGlobalResource().s_SamplerHeap.GetHeapPointer());

	gfxContext.SetConstantBuffer(0, GetGlobalResource().m_global_view_constant_buffer);
	gfxContext.SetConstantBuffer(1, GetGlobalResource().m_mesh_sdf_brick_tex_info);
	gfxContext.SetDynamicDescriptor(2, 0, m_sdf_instance_buffer.GetSRV());
	gfxContext.SetDynamicDescriptor(2, 1, GetGlobalResource().m_scene_sdf_infos_gpu.GetSRV());
	gfxContext.SetDescriptorTable(3, GetGlobalResource().s_TextureHeap[GetGlobalResource().m_global_sdf_brick_tex_table_idx]);
	gfxContext.SetDescriptorTable(4, GetGlobalResource().s_SamplerHeap[GetGlobalResource().m_global_sdf_brick_tex_sampler_table_idx]);

	gfxContext.SetVertexBuffer(0, m_sdf_vis_pos_buffer.VertexBufferView());
	gfxContext.SetVertexBuffer(1, m_sdf_vis_direction_buffer.VertexBufferView());
	gfxContext.SetIndexBuffer(m_sdf_vis_index_buffer.IndexBufferView());
	gfxContext.DrawIndexedInstanced(m_index_count_perinstance, m_instance_num, 0, 0, 0);
}

void CSimLumenVisualization::InitSDFVisBuffer()
{
	std::vector<Math::Vector3> global_vis_cube_positions;
	std::vector<Math::Vector3> global_vis_cube_direction;
	std::vector<unsigned int> global_vis_cube_indices;

	int global_index_offset = 0;
	for (int idx_x = 0; idx_x < 3; idx_x++)
	{
		for (int idx_y = 0; idx_y < 3; idx_y++)
		{
			for (int idx_z = 0; idx_z < 3; idx_z++)
			{
				if (idx_x == 1 && idx_y == 1 && idx_z == 1)
				{
					continue;
				}

				Math::Vector3 center_offset = (Math::Vector3(idx_x, idx_y, idx_z) - Math::Vector3(1, 1, 1)) * 0.375;
				std::vector<Math::Vector3> sub_positions;
				std::vector<unsigned int> sub_indices;
				CreateSDFVisBox(sub_positions, sub_indices, 0.25, 0.25, 0.25);
				for (int pos_idx = 0; pos_idx < sub_positions.size(); pos_idx++)
				{
					sub_positions[pos_idx] += center_offset;
					global_vis_cube_positions.push_back(sub_positions[pos_idx]);
					global_vis_cube_direction.push_back(center_offset);
				}

				for (int index_idx = 0; index_idx < sub_indices.size(); index_idx++)
				{

					global_vis_cube_indices.push_back(sub_indices[index_idx] + global_index_offset);
				}
				global_index_offset += sub_positions.size();
			}
		}
	}

	m_index_count_perinstance = global_vis_cube_indices.size();

	m_sdf_vis_pos_buffer.Create(L"m_sdf_vis_pos_buffer", global_vis_cube_positions.size(), sizeof(Math::Vector3), global_vis_cube_positions.data());
	m_sdf_vis_direction_buffer.Create(L"m_sdf_vis_direction_buffer", global_vis_cube_direction.size(), sizeof(Math::Vector3), global_vis_cube_direction.data());
	m_sdf_vis_index_buffer.Create(L"m_sdf_vis_index_buffer", global_vis_cube_indices.size(), sizeof(unsigned int), global_vis_cube_indices.data());

	std::vector<Matrix4> global_instances;
	for (int x_offset = -30; x_offset <= 30; x_offset += 10)
	{
		for (int y_offset = 0; y_offset <= 80; y_offset += 10)
		{
			for (int z_offset = 0; z_offset <= 300; z_offset += 10)
			{
				Matrix4 ins_local_to_world = Math::Matrix4(Math::AffineTransform(Vector3(x_offset, y_offset, -z_offset)));
				global_instances.push_back(ins_local_to_world);
			}
		}
	}

	m_instance_num = global_instances.size();
	m_sdf_instance_buffer.Create(L"m_sdf_instance_buffer", global_instances.size(), sizeof(Matrix4), global_instances.data());
}
