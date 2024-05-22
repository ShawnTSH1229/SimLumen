#pragma once
#include "SimLumenMeshInstance.h"
#include "../SimLumenCommon/ShaderCompile.h"

#define FORECE_REBUILD_RESOURCE 1
#define SDF_BRICK_TEX_SIZE 1024
#define SDF_BRICK_NUM_XY (SDF_BRICK_TEX_SIZE /g_brick_size)

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
	uint32_t texture_brick_num_x;
	uint32_t texture_brick_num_y;

	float padding0;
	float padding1;

	uint32_t texture_size_x;
	uint32_t texture_size_y;
	uint32_t texture_size_z;

	uint32_t scene_mesh_sdf_num;


	float padding2;
};
struct SSimLumenGlobalResource
{
	std::vector<SLumenMeshInstance> m_mesh_instances;
	
	CShaderCompiler m_shader_compiler;

	std::vector<SMeshSDFInfo> m_mesh_sdf_infos;
	StructuredBuffer m_scene_sdf_infos_gpu;

	TextureRef m_scene_mesh_sdf_brick_texture;
	uint32_t m_mesh_sdf_brick_tex_table_idx;
	uint32_t m_mesh_sdf_brick_tex__sampler_table_idx;

	D3D12_GPU_VIRTUAL_ADDRESS m_global_view_constant_buffer;
	D3D12_GPU_VIRTUAL_ADDRESS m_mesh_sdf_brick_tex_info;

	DescriptorHeap s_TextureHeap;
	DescriptorHeap s_SamplerHeap;
};

SSimLumenGlobalResource& GetGlobalResource();
void InitGlobalResource();
