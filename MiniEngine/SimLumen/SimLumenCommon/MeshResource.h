#pragma once
#include "SimLumenCommon.h"

static constexpr int g_brick_size = 8;

struct SDFBrickData
{
	uint8_t m_brick_data[g_brick_size][g_brick_size][g_brick_size];
};

struct SVolumeDFBuildData
{
	std::vector<SDFBrickData> distance_filed_volume;
	SBox m_LocalSpaceBox;
	float m_VoxelSize = 4.0;
	float m_max_distance;
	DirectX::XMUINT3 volume_brick_num_xyz;
};

struct SLumenMeshCards
{
	Math::BoundingBox m_local_boundbox;
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

	// x+ -> x-
	// x- -> x+
	
	// y+ -> y-
	// y- -> y+
	
	// z+ -> z-
	// z- -> z+

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