#include "SimLumenMeshInstance.h"
#include "TextureConvert.h"
#include "../SimLumenMeshBuilder/SimLumenObjLoader.h"

using namespace Graphics;

static void CreateBox(CSimLumenMeshResouce& cube,float x, float y, float z)
{
	float half_x = x / 2;
	float half_y = y / 2;
	float half_z = z / 2;

	cube.m_positions.resize(4 * 6);
	cube.m_normals.resize(4 * 6);
	cube.m_uvs.resize(4 * 6);
	cube.m_indices.resize(3 * 2 * 6);

	for (int face_idx = 0; face_idx < 6; face_idx++)
	{
		cube.m_indices[face_idx * 6u + 0u] = face_idx * 4 + 0;
		cube.m_indices[face_idx * 6u + 1u] = face_idx * 4 + 1;
		cube.m_indices[face_idx * 6u + 2u] = face_idx * 4 + 2;

		cube.m_indices[face_idx * 6u + 3u] = face_idx * 4 + 0;
		cube.m_indices[face_idx * 6u + 4u] = face_idx * 4 + 2;
		cube.m_indices[face_idx * 6u + 5u] = face_idx * 4 + 3;
	}

	for (int face_idx = 0; face_idx < 6; face_idx++)
	{
		cube.m_uvs[face_idx * 4 + 0] = DirectX::XMFLOAT2(0, 1);
		cube.m_uvs[face_idx * 4 + 1] = DirectX::XMFLOAT2(1, 1);
		cube.m_uvs[face_idx * 4 + 2] = DirectX::XMFLOAT2(1, 0);
		cube.m_uvs[face_idx * 4 + 3] = DirectX::XMFLOAT2(0, 0);
	}

	// right plane
	cube.m_positions[0] = DirectX::XMFLOAT3(half_x, half_y, -half_z);
	cube.m_positions[1] = DirectX::XMFLOAT3(half_x, half_y, half_z);
	cube.m_positions[2] = DirectX::XMFLOAT3(half_x, -half_y, half_z);
	cube.m_positions[3] = DirectX::XMFLOAT3(half_x, -half_y, -half_z);

	for (int idx = 0; idx < 4; idx++)
	{
		cube.m_normals[idx] = DirectX::XMFLOAT3(1, 0, 0);
	}

	// left plane
	cube.m_positions[4] = DirectX::XMFLOAT3(-half_x, half_y, half_z);
	cube.m_positions[5] = DirectX::XMFLOAT3(-half_x, half_y, -half_z);
	cube.m_positions[6] = DirectX::XMFLOAT3(-half_x, -half_y, -half_z);
	cube.m_positions[7] = DirectX::XMFLOAT3(-half_x, -half_y, half_z);

	for (int idx = 4; idx < 8; idx++)
	{
		cube.m_normals[idx] = DirectX::XMFLOAT3(-1, 0, 0);
	}

	// top plane
	cube.m_positions[8] = DirectX::XMFLOAT3(-half_x, half_y, half_z);
	cube.m_positions[9] = DirectX::XMFLOAT3(half_x, half_y, half_z);
	cube.m_positions[10] = DirectX::XMFLOAT3(half_x, half_y, -half_z);
	cube.m_positions[11] = DirectX::XMFLOAT3(-half_x, half_y, -half_z);

	for (int idx = 8; idx < 12; idx++)
	{
		cube.m_normals[idx] = DirectX::XMFLOAT3(0, 1, 0);
	}

	// bottom plane
	cube.m_positions[12] = DirectX::XMFLOAT3(-half_x, -half_y, -half_z);
	cube.m_positions[13] = DirectX::XMFLOAT3(half_x, -half_y, -half_z);
	cube.m_positions[14] = DirectX::XMFLOAT3(half_x, -half_y, half_z);
	cube.m_positions[15] = DirectX::XMFLOAT3(-half_x, -half_y, half_z);

	for (int idx = 12; idx < 16; idx++)
	{
		cube.m_normals[idx] = DirectX::XMFLOAT3(0, -1, 0);
	}

	// front plane
	cube.m_positions[16] = DirectX::XMFLOAT3(-half_x, half_y, -half_z);
	cube.m_positions[17] = DirectX::XMFLOAT3(half_x, half_y, -half_z);
	cube.m_positions[18] = DirectX::XMFLOAT3(half_x, -half_y, -half_z);
	cube.m_positions[19] = DirectX::XMFLOAT3(-half_x, -half_y, -half_z);

	for (int idx = 16; idx < 20; idx++)
	{
		cube.m_normals[idx] = DirectX::XMFLOAT3(0, 0, 1);
	}

	// back plane
	cube.m_positions[20] = DirectX::XMFLOAT3(half_x, half_y, half_z);
	cube.m_positions[21] = DirectX::XMFLOAT3(-half_x, half_y, half_z);
	cube.m_positions[22] = DirectX::XMFLOAT3(-half_x, -half_y, half_z);
	cube.m_positions[23] = DirectX::XMFLOAT3(half_x, -half_y, half_z);

	for (int idx = 20; idx < 24; idx++)
	{
		cube.m_normals[idx] = DirectX::XMFLOAT3(0, 0, -1);
	}
}

static void CreateBuffer(ByteAddressBuffer& out_buf, void* data, int num_element, int element_size)
{
	int buffer_size = num_element * element_size;
	UploadBuffer upload;
	upload.Create(L"None", buffer_size);
	memcpy(upload.Map(), data, buffer_size);
	upload.Unmap();
	out_buf.Create(L"Mesh Buffer", num_element, element_size, upload);
}

static void CreateBoxMeshResource(SLumenMeshInstance& mesh_instance, float x, float y, float z, float pos_x, float pos_y, float pos_z, float col_x, float col_y, float col_z)
{
	CSimLumenMeshResouce& mesh_resource = mesh_instance.m_mesh_resource;

	CreateBox(mesh_resource, x, y, z);
	mesh_resource.m_local_to_world = Math::Matrix4(Math::AffineTransform(Vector3(pos_x, pos_y, pos_z)));

	CreateBuffer(mesh_instance.m_vertex_pos_buffer, mesh_resource.m_positions.data(), mesh_resource.m_positions.size(), sizeof(DirectX::XMFLOAT3));
	CreateBuffer(mesh_instance.m_vertex_norm_buffer, mesh_resource.m_normals.data(), mesh_resource.m_normals.size(), sizeof(DirectX::XMFLOAT3));
	CreateBuffer(mesh_instance.m_vertex_uv_buffer, mesh_resource.m_uvs.data(), mesh_resource.m_uvs.size(), sizeof(DirectX::XMFLOAT2));
	CreateBuffer(mesh_instance.m_index_buffer, mesh_resource.m_indices.data(), mesh_resource.m_indices.size(), sizeof(unsigned int));

	mesh_instance.m_LumenConstant.WorldMatrix = mesh_resource.m_local_to_world;
	mesh_instance.m_LumenConstant.WorldIT = InverseTranspose(mesh_instance.m_LumenConstant.WorldMatrix.Get3x3());
	mesh_instance.m_LumenConstant.ColorMulti = DirectX::XMFLOAT4(col_x, col_y, col_z, 1);
}

//Right Handle
//https://learn.microsoft.com/en-us/windows/win32/direct3d9/coordinate-systems

void CreateDemoScene(std::vector<SLumenMeshInstance>& out_mesh_instances, DescriptorHeap& tex_heap, DescriptorHeap& sampler_heap)
{
	out_mesh_instances.resize(13);

	TextureRef cube_tex;
	uint32_t cube_tex_table_idx;
	uint32_t cube_sampler_table_idx;

	uint32_t DestCount = 1;
	uint32_t SourceCount = 1;

	int current_mesh_idx = 0;

	//texture
	{
		const std::wstring original_file = L"Assets/cube_tex.jpg";
		CompileTextureOnDemand(original_file, 0);
		std::wstring ddsFile = Utility::RemoveExtension(original_file) + L".dds";
		cube_tex = TextureManager::LoadDDSFromFile(ddsFile);

		DescriptorHandle texture_handles = tex_heap.Alloc(1);
		cube_tex_table_idx = tex_heap.GetOffsetOfHandle(texture_handles);

		D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[1];
		SourceTextures[0] = cube_tex.GetSRV();

		g_Device->CopyDescriptors(1, &texture_handles, &DestCount, DestCount, SourceTextures, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	//sampler
	{
		DescriptorHandle SamplerHandles = sampler_heap.Alloc(1);
		cube_sampler_table_idx = sampler_heap.GetOffsetOfHandle(SamplerHandles);

		D3D12_CPU_DESCRIPTOR_HANDLE SourceSamplers[1] = { SamplerLinearWrap };

		g_Device->CopyDescriptors(1, &SamplerHandles, &DestCount, DestCount, SourceSamplers, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	for (int idx = 0; idx < out_mesh_instances.size(); idx++)
	{
		CSimLumenMeshResouce& mesh_resource = out_mesh_instances[idx].m_mesh_resource;
		SLumenMeshInstance& mesh_instance = out_mesh_instances[idx];

		mesh_instance.m_tex = cube_tex;
		mesh_instance.m_tex_table_idx = cube_tex_table_idx;
		mesh_instance.m_sampler_table_idx = cube_sampler_table_idx;
	}

	{
		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			5, 64, 100,
			-30, 32, -50,
			1, 0.2, 0.2);

		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			5, 64, 100,
			-30, 32, -50 - 10 - 120,
			1.0, 1.0, 0.2);
	}

	{
		TextureRef mesh_tex;
		uint32_t mesh_tex_table_idx;
		uint32_t mesh_sampler_table_idx;

		//texture
		{
			const std::wstring original_file = L"Assets/erato-101.jpg";
			CompileTextureOnDemand(original_file, 0);
			std::wstring ddsFile = Utility::RemoveExtension(original_file) + L".dds";
			mesh_tex = TextureManager::LoadDDSFromFile(ddsFile);

			DescriptorHandle texture_handles = tex_heap.Alloc(1);
			mesh_tex_table_idx = tex_heap.GetOffsetOfHandle(texture_handles);

			D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[1];
			SourceTextures[0] = mesh_tex.GetSRV();

			g_Device->CopyDescriptors(1, &texture_handles, &DestCount, DestCount, SourceTextures, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		SLumenMeshInstance& mesh_instance = out_mesh_instances[current_mesh_idx++];
		CSimLumenMeshResouce& mesh_resource = mesh_instance.m_mesh_resource;
		LoadObj(std::string("Assets/erato.obj"), &mesh_resource);

		CreateBuffer(mesh_instance.m_vertex_pos_buffer, mesh_resource.m_positions.data(), mesh_resource.m_positions.size(), sizeof(DirectX::XMFLOAT3));
		CreateBuffer(mesh_instance.m_vertex_norm_buffer, mesh_resource.m_normals.data(), mesh_resource.m_normals.size(), sizeof(DirectX::XMFLOAT3));
		CreateBuffer(mesh_instance.m_vertex_uv_buffer, mesh_resource.m_uvs.data(), mesh_resource.m_uvs.size(), sizeof(DirectX::XMFLOAT2));
		CreateBuffer(mesh_instance.m_index_buffer, mesh_resource.m_indices.data(), mesh_resource.m_indices.size(), sizeof(unsigned int));

		mesh_instance.m_tex = mesh_tex;
		mesh_instance.m_tex_table_idx = mesh_tex_table_idx;
		mesh_instance.m_sampler_table_idx = cube_sampler_table_idx;

		mesh_resource.m_volume_df_data.m_VoxelSize = 0.5;
		mesh_resource.m_local_to_world = Math::Matrix4(Math::AffineTransform(Vector3(-10, 0, -50)));

		mesh_instance.m_LumenConstant.WorldMatrix = mesh_resource.m_local_to_world;
		mesh_instance.m_LumenConstant.WorldIT = InverseTranspose(mesh_instance.m_LumenConstant.WorldMatrix.Get3x3());
		mesh_instance.m_LumenConstant.ColorMulti = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1);

		mesh_resource.LoadFrom(std::wstring(L"Assets/erato.obj"));
	}




	{
		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			5, 64, 120,
			30, 32, -60,
			0.2, 1.0, 0.2);

		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			5, 64, 120,
			30, 32, -60 - 120,
			0.2, 0.2, 1.0);
	}

	{
		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			60 + 5, 5, 120,
			0, -2.5, -60,
			1.0, 1.0, 1.0);

		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			60 + 5, 5, 120,
			0, -2.5, -60 - 120,
			1.0, 1.0, 1.0);
	}

	{
		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			60 + 5, 5, 60,
			0, 66.5, -90,
			1.0, 1.0, 1.0);

		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			60 + 5, 5, 120,
			0, 66.5, -60 - 120,
			1.0, 1.0, 1.0);
	}

	{
		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			35, 64, 5,
			16.5, 32, -240,
			1.0, 1.0, 1.0);
	}

	{
		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			20, 20, 20,
			15, 20, -200,
			1.0, 1.0, 1.0);
	}

	{
		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			20, 20, 20,
			10, 20, -80,
			1.0, 1.0, 1.0);
	}

	{
		CreateBoxMeshResource(
			out_mesh_instances[current_mesh_idx++],
			20, 20, 20,
			-10, 36, -150,
			1.0, 1.0, 1.0);
	}


}