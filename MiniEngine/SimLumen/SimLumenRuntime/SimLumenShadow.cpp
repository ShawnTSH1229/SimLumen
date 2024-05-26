#include "SimLumenShadow.h"

using namespace Graphics;

void CSimLumenShadow::Init()
{
	m_SunShadow.UpdateMatrix(
		-GetGlobalResource().m_LightDirection,
		Vector3(gloabl_sdf_center) - GetGlobalResource().m_LightDirection * 200,
		Vector3(SIMLUMEN_SHADOW_DIMENSION, SIMLUMEN_SHADOW_DIMENSION, SIMLUMEN_SHADOW_DIMENSION),
		(uint32_t)g_ShadowBuffer.GetWidth(), (uint32_t)g_ShadowBuffer.GetHeight(), 16);

	GetGlobalResource().m_shadow_vpmatrix = m_SunShadow.GetViewProjMatrix();

	m_shadow_rendering_sig.Reset(2, 1);
	m_shadow_rendering_sig.InitStaticSampler(0, SamplerLinearClampDesc);
	m_shadow_rendering_sig[0].InitAsConstantBuffer(0);
	m_shadow_rendering_sig[1].InitAsConstantBuffer(1);
	m_shadow_rendering_sig.Finalize(L"m_shadow_rendering_sig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_INPUT_ELEMENT_DESC pos[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenShadowMapRendering.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
	std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/SimLumenShadowMapRendering.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

	m_shadow_rendering_pso = GraphicsPSO(L"m_shadow_rendering_pso");
	m_shadow_rendering_pso.SetRootSignature(m_shadow_rendering_sig);
	m_shadow_rendering_pso.SetRasterizerState(RasterizerDefault);
	m_shadow_rendering_pso.SetBlendState(BlendDisable);
	m_shadow_rendering_pso.SetDepthStencilState(DepthStateReadWrite);
	m_shadow_rendering_pso.SetInputLayout(_countof(pos), pos);
	m_shadow_rendering_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_shadow_rendering_pso.SetDepthTargetFormat(g_ShadowBuffer.GetFormat());
	m_shadow_rendering_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
	m_shadow_rendering_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
	m_shadow_rendering_pso.Finalize();
}

void CSimLumenShadow::RenderingShadowMap(GraphicsContext& gfxContext)
{
	EngineProfiling::BeginBlock(L"RenderingShadowMap");
	gfxContext.SetRootSignature(m_shadow_rendering_sig);
	gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gfxContext.SetPipelineState(m_shadow_rendering_pso);
	gfxContext.SetConstantBuffer(1, GetGlobalResource().m_global_view_constant_buffer);

	g_ShadowBuffer.BeginRendering(gfxContext);
	std::vector<SLumenMeshInstance>& mesh_instances = GetGlobalResource().m_mesh_instances;
	for (int mesh_idx = 0; mesh_idx < mesh_instances.size(); mesh_idx++)
	{
		SLumenMeshInstance& lumen_mesh_instance = GetGlobalResource().m_mesh_instances[mesh_idx];
		DynAlloc mesh_consatnt = gfxContext.ReserveUploadMemory(sizeof(SLumenMeshConstant));
		memcpy(mesh_consatnt.DataPtr, &lumen_mesh_instance.m_LumenConstant, sizeof(SLumenMeshConstant));
		gfxContext.SetConstantBuffer(0, mesh_consatnt.GpuAddress);

		gfxContext.SetVertexBuffer(0, lumen_mesh_instance.m_vertex_pos_buffer.VertexBufferView());
		gfxContext.SetIndexBuffer(lumen_mesh_instance.m_index_buffer.IndexBufferView());
		gfxContext.DrawIndexed(lumen_mesh_instance.m_mesh_resource.m_indices.size());
	}
	g_ShadowBuffer.EndRendering(gfxContext);
	EngineProfiling::EndBlock();
}
