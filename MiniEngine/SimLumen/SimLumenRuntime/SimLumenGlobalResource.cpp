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

	gSimLumenGlobalResource.m_scene_mesh_sdf_brick_texture = TextureManager::LoadDDSFromFile(sdf_file);
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

	gSimLumenGlobalResource.m_global_sdf_brick_texture = TextureManager::LoadDDSFromFile(global_sdf_file);
}


void InitSceneCardInfo()
{
	std::vector<SLumenMeshInstance>& scene_meshs = gSimLumenGlobalResource.m_mesh_instances;
	std::vector<SCardInfo>& scene_card_infos = gSimLumenGlobalResource.m_scene_card_info;
	for (int mesh_idx = 0; mesh_idx < scene_meshs.size(); mesh_idx++)
	{
		SLumenMeshInstance& mesh = scene_meshs[mesh_idx];
		CSimLumenMeshResouce& mesh_resource = mesh.m_mesh_resource;

		for (int dir = 0; dir < 6; dir++)
		{
			SCardInfo card_info;
			card_info.rotate_back_matrix = mesh_resource.m_cards[dir].m_rotate_back_matrix;
			card_info.rotate_matrix = Math::Invert(card_info.rotate_back_matrix);
			card_info.rotated_extents = mesh_resource.m_cards[dir].m_rotated_extents;
			card_info.bound_center = mesh_resource.m_cards[dir].m_bound_center;
			card_info.mesh_index = mesh_idx;
			card_info.local_to_world = mesh_resource.m_local_to_world;
			card_info.world_to_local = Math::Invert(card_info.local_to_world);
			scene_card_infos.push_back(card_info);
		}
	};
	gSimLumenGlobalResource.m_scene_card_infos_gpu.Create(L"m_scene_card_infos_gpu", scene_card_infos.size(), sizeof(SCardInfo), scene_card_infos.data());
}

void InitFinalGatherResource()
{
	uint32_t screen_width = g_SceneColorBuffer.GetWidth();
	uint32_t screen_height = g_SceneColorBuffer.GetHeight();
	
	uint32_t screen_probe_num_x = (screen_width + SCREEN_SPACE_PROBE_SIZE - 1) / SCREEN_SPACE_PROBE_SIZE;
	uint32_t screen_probe_num_y = (screen_height + SCREEN_SPACE_PROBE_SIZE - 1) / SCREEN_SPACE_PROBE_SIZE;
	
	gSimLumenGlobalResource.m_lumen_scene_info.screen_probe_size_x = screen_probe_num_x;
	gSimLumenGlobalResource.m_lumen_scene_info.screen_probe_size_y = screen_probe_num_y;
	
	gSimLumenGlobalResource.m_lumen_scene_info.is_pdf_thread_size_x = screen_probe_num_x * SCREEN_SPACE_PROBE_SIZE;
	gSimLumenGlobalResource.m_lumen_scene_info.is_pdf_thread_size_y = screen_probe_num_y * SCREEN_SPACE_PROBE_SIZE;

	gSimLumenGlobalResource.m_brdf_pdf_visualize.Create(L"m_brdf_pdf_visualize", screen_width, screen_height, 1, DXGI_FORMAT_R16_FLOAT);
	gSimLumenGlobalResource.m_brdf_pdf_sh.Create(L"m_brdf_pdf_sh", screen_probe_num_x * screen_probe_num_y, sizeof(float));
}

void InitGlobalResource()
{
	InitMeshSDFs();
	InitGlobalSDF();
	InitSceneCardInfo();
	InitFinalGatherResource();

	XMFLOAT3 vtx_pos[6] = {
	XMFLOAT3(1,0,0),XMFLOAT3(1,1,0),XMFLOAT3(0,0,0),
	XMFLOAT3(1,1,0), XMFLOAT3(0,1,0), XMFLOAT3(0,0,0) };
	gSimLumenGlobalResource.m_full_screen_pos_buffer.Create(L"m_full_screen_pos_buffer", 6, sizeof(XMFLOAT3), vtx_pos);

	XMFLOAT2 vtx_uv[6] = {
	 XMFLOAT2(1,1),XMFLOAT2(1,0),XMFLOAT2(0,1),
	 XMFLOAT2(1,0) , XMFLOAT2(0,0),XMFLOAT2(0,1) };

	gSimLumenGlobalResource.m_full_screen_uv_buffer.Create(L"m_full_screen_uv_buffer", 6, sizeof(XMFLOAT2), vtx_uv);

	gSimLumenGlobalResource.m_LightDirection = Normalize(Math::Vector3(-1, 1, -1));
	gSimLumenGlobalResource.m_atlas_size = Math::XMINT2(2048, 2048);
	gSimLumenGlobalResource.m_atlas_num_xy = Math::XMINT2(2048 / 128, 2048 / 128);

	gSimLumenGlobalResource.m_lumen_scene_info.scene_voxel_min_pos = DirectX::XMFLOAT3(gloabl_sdf_center.x - gloabl_sdf_extent.x, gloabl_sdf_center.y - gloabl_sdf_extent.y, gloabl_sdf_center.z - gloabl_sdf_extent.z);
	gSimLumenGlobalResource.m_lumen_scene_info.scene_voxel_max_pos = DirectX::XMFLOAT3(gloabl_sdf_center.x + gloabl_sdf_extent.x, gloabl_sdf_center.y + gloabl_sdf_extent.y, gloabl_sdf_center.z + gloabl_sdf_extent.z);
	gSimLumenGlobalResource.m_lumen_scene_info.voxel_size = gloabl_sdf_voxel_size * 2;
	gSimLumenGlobalResource.m_lumen_scene_info.card_num_xy = GetGlobalResource().m_atlas_num_xy.x;
	gSimLumenGlobalResource.m_lumen_scene_info.scene_card_num = GetGlobalResource().m_scene_card_info.size();
	gSimLumenGlobalResource.m_lumen_scene_info.frame_num = 0;

	gSimLumenGlobalResource.scene_voxel_visibility_buffer.Create(L"scene_voxel_visibility_buffer", SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y * SCENE_VOXEL_SIZE_Z, sizeof(SVoxelVisibilityInfo));
	gSimLumenGlobalResource.m_scene_voxel_lighting.Create(L"m_scene_voxel_lighting", SCENE_VOXEL_SIZE_X * SCENE_VOXEL_SIZE_Y * SCENE_VOXEL_SIZE_Z, sizeof(SVoxelLighting));
}
