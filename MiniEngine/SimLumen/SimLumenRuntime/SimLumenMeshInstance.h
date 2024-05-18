#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/MeshResource.h"

__declspec(align(256)) struct SLumenViewGlobalConstant
{
	Math::Matrix4 ViewProjMatrix;
	Math::Vector3 CameraPos;
	Math::Vector3 SunDirection;
	Math::Vector3 SunIntensity;
};

__declspec(align(256)) struct SLumenMeshConstant
{
	Math::Matrix4 WorldMatrix;
	Math::Matrix3 WorldIT;
	Math::Vector4 ColorMulti = DirectX::XMFLOAT4(1, 1, 1, 1);
};

struct SLumenMeshInstance
{
	CSimLumenMeshResouce m_mesh_resource;

	ByteAddressBuffer m_vertex_pos_buffer;
	ByteAddressBuffer m_vertex_norm_buffer;
	ByteAddressBuffer m_vertex_uv_buffer;
	ByteAddressBuffer m_index_buffer;

	SLumenMeshConstant m_LumenConstant;

	TextureRef m_tex;
	uint32_t m_tex_table_idx;
	uint32_t m_sampler_table_idx;
};

void CreateDemoScene(std::vector<SLumenMeshInstance>& out_mesh_instances, DescriptorHeap& tex_heap, DescriptorHeap& sampler_heap);