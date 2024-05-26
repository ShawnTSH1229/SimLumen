#pragma once
#include "SimLumenMeshInstance.h"
#include "../SimLumenCommon/ShaderCompile.h"

#define FORECE_REBUILD_RESOURCE 1
#define SDF_BRICK_TEX_SIZE 1024
#define SDF_BRICK_NUM_XY (SDF_BRICK_TEX_SIZE /g_brick_size)

#define SCENE_VOXEL_SIZE_X 48
#define SCENE_VOXEL_SIZE_Y 48
#define SCENE_VOXEL_SIZE_Z 160

#define SIMLUMEN_SHADOW_DIMENSION 384

struct SMeshSDFInfo
{
	Math::Matrix4 volume_to_world;
	Math::Matrix4 world_to_volume;

	DirectX::XMFLOAT3 volume_position_center;
	DirectX::XMFLOAT3 volume_position_extent; // volume space

	DirectX::XMUINT3 volume_brick_num_xyz;

	float volume_brick_size;
	uint32_t volume_brick_start_idx;

	float sdf_distance_scale_x; // x : 2 * max distance , y : - max distance
	float sdf_distance_scale_y; // x : 2 * max distance , y : - max distance

	uint32_t mesh_card_start_index;

	float padding_0;
	float padding_1;
};

static_assert(sizeof(SMeshSDFInfo) == sizeof(float) * (16 + 16 + 3 + 3 + 3 + 7),"aa");

__declspec(align(256)) struct SMeshSdfBrickTextureInfo
{
	// scene sdf infomation
	uint32_t texture_brick_num_x;
	uint32_t texture_brick_num_y;

	float padding0;
	float padding1;

	uint32_t texture_size_x;
	uint32_t texture_size_y;
	uint32_t texture_size_z;

	uint32_t scene_mesh_sdf_num;

	//global infomation
	float gloabl_sdf_voxel_size;
	DirectX::XMFLOAT3 gloabl_sdf_center;

	DirectX::XMFLOAT3 global_sdf_extents;
	float global_sdf_scale_x;

	DirectX::XMFLOAT3 global_sdf_tex_size_xyz;
	float global_sdf_scale_y;
};

__declspec(align(256)) struct SLumenSceneInfo
{
	DirectX::XMFLOAT3 scene_voxel_min_pos;
	float padding;

	DirectX::XMFLOAT3 scene_voxel_max_pos;
	float voxel_size;

	uint32_t card_num_xy;
	uint32_t scene_card_num;
};

struct SVoxelDirVisInfo
{
	int mesh_index; // -1: invalid direction
	float hit_distance;
};

struct SVoxelVisibilityInfo
{
	SVoxelDirVisInfo voxel_vis_info[6];
};

struct SCardInfo
{
	Math::Matrix4 local_to_world;
	Math::Matrix4 rotate_back_matrix;
	DirectX::XMFLOAT3 rotated_extents;
	float padding0;
	DirectX::XMFLOAT3 bound_center;
	uint32_t mesh_index;
};

struct SSimLumenGlobalResource
{
	std::vector<SLumenMeshInstance> m_mesh_instances;
	std::vector<uint8_t> global_sdf_data;

	CShaderCompiler m_shader_compiler;

	// sdf
	std::vector<SMeshSDFInfo> m_mesh_sdf_infos;
	StructuredBuffer m_scene_sdf_infos_gpu;

	TextureRef m_scene_mesh_sdf_brick_texture;
	TextureRef m_global_sdf_brick_texture;

	// scene card info
	std::vector<SCardInfo> m_scene_card_info;
	StructuredBuffer m_scene_card_infos_gpu;

	// voxel vis
	SLumenSceneInfo m_lumen_scene_info;

	D3D12_GPU_VIRTUAL_ADDRESS m_global_view_constant_buffer;
	D3D12_GPU_VIRTUAL_ADDRESS m_mesh_sdf_brick_tex_info;

	ByteAddressBuffer m_full_screen_pos_buffer;
	ByteAddressBuffer m_full_screen_uv_buffer;

	Math::Vector3 m_LightDirection;
	Math::XMINT2 m_atlas_size;
	Math::XMINT2 m_atlas_num_xy;

	StructuredBuffer scene_voxel_visibility_buffer;

	Math::Matrix4 m_shadow_vpmatrix;

	// 1: visualize mesh sdf normal
	// 2: visualize global sdf normal
	int m_visualize_type;
};

SSimLumenGlobalResource& GetGlobalResource();
void InitGlobalResource();
