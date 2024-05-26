#pragma once
#include "SimLumenCommon.h"

static constexpr int g_brick_size = 8;

static constexpr int global_sdf_size_x = 96;
static constexpr int global_sdf_size_y = 96;
static constexpr int global_sdf_size_z = 320;

static constexpr int gloabl_sdf_voxel_size = 1;
static constexpr DirectX::XMFLOAT3 gloabl_sdf_center = DirectX::XMFLOAT3(0, 40, -120);
static constexpr DirectX::XMFLOAT3 gloabl_sdf_extent = DirectX::XMFLOAT3(48, 48, 160);
static constexpr float gloabl_sdf_scale_x = 100.0;
static constexpr float gloabl_sdf_scale_y = -30.0;

struct SDFBrickData
{
	uint8_t m_brick_data[g_brick_size][g_brick_size][g_brick_size];
};

struct SVolumeDFBuildData
{
	std::vector<SDFBrickData> distance_filed_volume;
	SBox m_LocalSpaceBox;
	float m_VoxelSize = 2.0;
	float m_max_distance;
	DirectX::XMUINT3 volume_brick_num_xyz;
};

struct SLumenMeshCards
{
	Math::BoundingBox m_local_boundbox;

	Math::Matrix4 m_rotate_back_matrix;
	DirectX::XMFLOAT3 m_rotated_extents;
	DirectX::XMFLOAT3 m_bound_center;
};

struct SSimLumenResourceHeader
{
	int volume_array_size;
};

class CSimLumenMeshResouce
{
public:
	SSimLumenResourceHeader m_header;
	SVolumeDFBuildData m_volume_df_data;

	SLumenMeshCards m_cards[6];

	std::vector<DirectX::XMFLOAT3> m_positions;
	std::vector<DirectX::XMFLOAT3> m_normals;
	std::vector<DirectX::XMFLOAT2> m_uvs;
	std::vector<unsigned int> m_indices;

	Math::BoundingBox m_BoundingBox;

	Math::Matrix4 m_local_to_world;

	bool m_need_rebuild = true;
	bool m_need_save= false;
	void LoadFrom(const std::wstring& source_file, bool bforce_rebuild = false);
	void SaveTo(const std::string& source_file_an);
};