#include "SimLumenGlobalResource.h"
#include "../Core/Utility.h"
#include "DirectXTex.h"

using namespace Graphics;

static SSimLumenGlobalResource gSimLumenGlobalResource;

SSimLumenGlobalResource& GetGlobalResource()
{
	return gSimLumenGlobalResource;
}

static void InitMeshSDFs()
{
	std::vector<SLumenMeshInstance>& scene_meshs = gSimLumenGlobalResource.m_mesh_instances;
	std::vector<SMeshSDFInfo>& mesh_sdf_infos = gSimLumenGlobalResource.m_mesh_sdf_infos;
	mesh_sdf_infos.resize(scene_meshs.size());

	std::vector<uint8_t> slice_datas[g_brick_size];

	for (int sli_idx = 0; sli_idx < g_brick_size; sli_idx++)
	{
		slice_datas[sli_idx].resize(SDF_BRICK_TEX_SIZE * SDF_BRICK_TEX_SIZE);
		memset(slice_datas[sli_idx].data(), 0, SDF_BRICK_TEX_SIZE * SDF_BRICK_TEX_SIZE);
	}

	uint32_t global_brick_offset = 0;
	for (int mesh_idx = 0; mesh_idx < scene_meshs.size(); mesh_idx++)
	{
		SLumenMeshInstance& mesh = scene_meshs[mesh_idx];
		CSimLumenMeshResouce& mesh_resource = mesh.m_mesh_resource;
		std::vector<SDFBrickData>& brick_datas = mesh_resource.m_volume_df_data.distance_filed_volume;

		DirectX::XMFLOAT3 center_pos = DirectX::XMFLOAT3(
			(mesh_resource.m_volume_df_data.m_LocalSpaceBox.volume_max_pos.x + mesh_resource.m_volume_df_data.m_LocalSpaceBox.volume_min_pos.x) * 0.5,
			(mesh_resource.m_volume_df_data.m_LocalSpaceBox.volume_max_pos.y + mesh_resource.m_volume_df_data.m_LocalSpaceBox.volume_min_pos.y) * 0.5,
			(mesh_resource.m_volume_df_data.m_LocalSpaceBox.volume_max_pos.z + mesh_resource.m_volume_df_data.m_LocalSpaceBox.volume_min_pos.z) * 0.5
		);

		DirectX::XMFLOAT3 extent_pos = DirectX::XMFLOAT3(
			(mesh_resource.m_volume_df_data.m_LocalSpaceBox.volume_max_pos.x - center_pos.x),
			(mesh_resource.m_volume_df_data.m_LocalSpaceBox.volume_max_pos.y - center_pos.y),
			(mesh_resource.m_volume_df_data.m_LocalSpaceBox.volume_max_pos.z - center_pos.z)
		);

		mesh_sdf_infos[mesh_idx].volume_position_center = center_pos;
		mesh_sdf_infos[mesh_idx].volume_position_extent = extent_pos;
		mesh_sdf_infos[mesh_idx].volume_to_world = mesh.m_LumenConstant.WorldMatrix;
		mesh_sdf_infos[mesh_idx].world_to_volume = Math::Invert(mesh_sdf_infos[mesh_idx].volume_to_world);
		mesh_sdf_infos[mesh_idx].volume_brick_num_xyz = mesh_resource.m_volume_df_data.volume_brick_num_xyz;
		mesh_sdf_infos[mesh_idx].volume_brick_size = g_brick_size * mesh_resource.m_volume_df_data.m_VoxelSize;
		mesh_sdf_infos[mesh_idx].volume_brick_start_idx = global_brick_offset;
		mesh_sdf_infos[mesh_idx].sdf_distance_scale_x = mesh_resource.m_volume_df_data.m_max_distance * 2.0;
		mesh_sdf_infos[mesh_idx].sdf_distance_scale_y = -mesh_resource.m_volume_df_data.m_max_distance;
		mesh_sdf_infos[mesh_idx].mesh_card_start_index = mesh_idx * 6;

		for (int brick_idx = 0; brick_idx < brick_datas.size(); brick_idx++)
		{
			int global_brick_index = global_brick_offset + brick_idx;

			int brick_index_x = global_brick_index % SDF_BRICK_NUM_XY;
			int brick_index_y = global_brick_index / SDF_BRICK_NUM_XY;

			int brick_tex_offset_x = brick_index_x * g_brick_size;
			int brick_tex_offset_y = brick_index_y * g_brick_size;

			SDFBrickData& brick_data = brick_datas[brick_idx];
			for (int vox_idx_z = 0; vox_idx_z < g_brick_size; vox_idx_z++)
			{
				for (int vox_idx_x = 0; vox_idx_x < g_brick_size; vox_idx_x++)
				{
					for (int vox_idx_y = 0; vox_idx_y < g_brick_size; vox_idx_y++)
					{
						int tex_index_x = brick_tex_offset_x + vox_idx_x;
						int tex_index_y = brick_tex_offset_y + vox_idx_y;
						int dest_index = tex_index_y * SDF_BRICK_TEX_SIZE + tex_index_x;

						slice_datas[vox_idx_z][dest_index] = brick_data.m_brick_data[vox_idx_x][vox_idx_y][vox_idx_z];
					}
				}
			}
		}

		global_brick_offset += brick_datas.size();
	}
	gSimLumenGlobalResource.m_scene_sdf_infos_gpu.Create(L"m_scene_sdf_infos_gpu", gSimLumenGlobalResource.m_mesh_sdf_infos.size(),sizeof(SMeshSDFInfo), gSimLumenGlobalResource.m_mesh_sdf_infos.data());

	Image images[g_brick_size];
	for (int slice_idx = 0; slice_idx < g_brick_size; slice_idx++)
	{
		images[slice_idx].width = SDF_BRICK_TEX_SIZE;
		images[slice_idx].height = SDF_BRICK_TEX_SIZE;
		images[slice_idx].format = DXGI_FORMAT_R8_UNORM;
		images[slice_idx].rowPitch = SDF_BRICK_TEX_SIZE;
		images[slice_idx].slicePitch = SDF_BRICK_TEX_SIZE * SDF_BRICK_TEX_SIZE;
		images[slice_idx].pixels = slice_datas[slice_idx].data();
	}

	const std::wstring sdf_file = L"Assets/sdf_brick_tex.dds";

	std::unique_ptr<ScratchImage> image(new ScratchImage);
	image->Initialize3DFromImages(images, g_brick_size);
	HRESULT hr = SaveToDDSFile(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DDS_FLAGS_NONE, sdf_file.c_str());
	if (FAILED(hr))
	{
		Utility::Printf("Write file false.\n");
	}

	uint32_t DestCount = 1;
	uint32_t SourceCount = 1;

	{
		gSimLumenGlobalResource.m_scene_mesh_sdf_brick_texture = TextureManager::LoadDDSFromFile(sdf_file);
		DescriptorHandle texture_handles = GetGlobalResource().s_TextureHeap.Alloc(1);
		gSimLumenGlobalResource.m_mesh_sdf_brick_tex_table_idx = GetGlobalResource().s_TextureHeap.GetOffsetOfHandle(texture_handles);

		D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[1];
		SourceTextures[0] = gSimLumenGlobalResource.m_scene_mesh_sdf_brick_texture.GetSRV();

		g_Device->CopyDescriptors(1, &texture_handles, &DestCount, DestCount, SourceTextures, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	//sampler
	{
		DescriptorHandle SamplerHandles = GetGlobalResource().s_SamplerHeap.Alloc(1);
		gSimLumenGlobalResource.m_mesh_sdf_brick_tex_sampler_table_idx = GetGlobalResource().s_SamplerHeap.GetOffsetOfHandle(SamplerHandles);

		D3D12_CPU_DESCRIPTOR_HANDLE SourceSamplers[1] = { SamplerPointClamp };

		g_Device->CopyDescriptors(1, &SamplerHandles, &DestCount, DestCount, SourceSamplers, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

}

void InitGlobalSDF()
{
	Image images[global_sdf_size_z];
	for (int slice_idx = 0; slice_idx < global_sdf_size_z; slice_idx++)
	{
		images[slice_idx].width = global_sdf_size_x;
		images[slice_idx].height = global_sdf_size_y;
		images[slice_idx].format = DXGI_FORMAT_R8_UNORM;
		images[slice_idx].rowPitch = global_sdf_size_x;
		images[slice_idx].slicePitch = global_sdf_size_x * global_sdf_size_y;
		images[slice_idx].pixels = gSimLumenGlobalResource.global_sdf_data.data() + slice_idx * global_sdf_size_x * global_sdf_size_y;
	}

	const std::wstring global_sdf_file = L"Assets/global_sdf_tex.dds";

	std::unique_ptr<ScratchImage> image(new ScratchImage);
	image->Initialize3DFromImages(images, global_sdf_size_z);
	HRESULT hr = SaveToDDSFile(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DDS_FLAGS_NONE, global_sdf_file.c_str());

	if (FAILED(hr))
	{
		Utility::Printf("Write file false.\n");
	}

	uint32_t DestCount = 1;
	uint32_t SourceCount = 1;

	{
		gSimLumenGlobalResource.m_global_sdf_brick_texture = TextureManager::LoadDDSFromFile(global_sdf_file);
		DescriptorHandle texture_handles = GetGlobalResource().s_TextureHeap.Alloc(1);
		gSimLumenGlobalResource.m_global_sdf_brick_tex_table_idx = GetGlobalResource().s_TextureHeap.GetOffsetOfHandle(texture_handles);

		D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[1];
		SourceTextures[0] = gSimLumenGlobalResource.m_global_sdf_brick_texture.GetSRV();

		g_Device->CopyDescriptors(1, &texture_handles, &DestCount, DestCount, SourceTextures, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	//sampler
	{
		DescriptorHandle SamplerHandles = GetGlobalResource().s_SamplerHeap.Alloc(1);
		gSimLumenGlobalResource.m_global_sdf_brick_tex_sampler_table_idx = GetGlobalResource().s_SamplerHeap.GetOffsetOfHandle(SamplerHandles);

		D3D12_CPU_DESCRIPTOR_HANDLE SourceSamplers[1] = { SamplerPointClamp };

		g_Device->CopyDescriptors(1, &SamplerHandles, &DestCount, DestCount, SourceSamplers, &SourceCount, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}
}

void InitGlobalResource()
{
	InitMeshSDFs();
	InitGlobalSDF();

	XMFLOAT3 vtx_pos[6] = {
	XMFLOAT3(1,0,0),XMFLOAT3(1,1,0),XMFLOAT3(0,0,0),
	XMFLOAT3(1,1,0), XMFLOAT3(0,1,0), XMFLOAT3(0,0,0) };
	gSimLumenGlobalResource.m_full_screen_pos_buffer.Create(L"m_full_screen_pos_buffer", 6, sizeof(XMFLOAT3), vtx_pos);

	//XMFLOAT2 vtx_uv[6] = {
	// XMFLOAT2(1,0),XMFLOAT2(1,1),XMFLOAT2(0,0),
	// XMFLOAT2(1,1) , XMFLOAT2(0,1),XMFLOAT2(0,0) };
	XMFLOAT2 vtx_uv[6] = {
	 XMFLOAT2(1,1),XMFLOAT2(1,0),XMFLOAT2(0,1),
	 XMFLOAT2(1,0) , XMFLOAT2(0,0),XMFLOAT2(0,1) };

	gSimLumenGlobalResource.m_full_screen_uv_buffer.Create(L"m_full_screen_uv_buffer", 6, sizeof(XMFLOAT2), vtx_uv);

	gSimLumenGlobalResource.m_LightDirection = Normalize(Math::Vector3(-1, 1, 1));
	gSimLumenGlobalResource.m_atlas_size = Math::XMINT2(2048, 2048);
	gSimLumenGlobalResource.m_atlas_num_xy = Math::XMINT2(2048 / 128, 2048 / 128);

	gSimLumenGlobalResource.m_scene_voxel_vis_info.scene_voxel_min_pos = DirectX::XMFLOAT3(gloabl_sdf_center.x - gloabl_sdf_extent.x, gloabl_sdf_center.y - gloabl_sdf_extent.y, gloabl_sdf_center.z - gloabl_sdf_extent.z);
	gSimLumenGlobalResource.m_scene_voxel_vis_info.scene_voxel_max_pos = DirectX::XMFLOAT3(gloabl_sdf_center.x + gloabl_sdf_extent.x, gloabl_sdf_center.y + gloabl_sdf_extent.y, gloabl_sdf_center.z + gloabl_sdf_extent.z);
	gSimLumenGlobalResource.m_scene_voxel_vis_info.voxel_size = gloabl_sdf_voxel_size * 2;

	gSimLumenGlobalResource.scene_voxel_visibility_buffer.Create(L"scene_voxel_visibility_buffer", SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y * SCENE_VOXEL_SIZE_Z, sizeof(SVoxelVisibilityInfo));
}
