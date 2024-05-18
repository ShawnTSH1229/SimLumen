#pragma once
#include "SimLumenCommon.h"

struct SVolumeDFBuildData
{
	SBox m_LocalSpaceBox;
	float m_VoxelSize = 4.0;//todo:fix bug
	std::vector<uint8_t> distance_filed_volume;
};


struct SLumenMeshCards
{
	Math::BoundingBox m_local_boundbox;
};

class CSimLumenMeshResouce
{
public:
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

	void LoadFrom(const std::string& source_file_an, const std::wstring& source_file, bool bforce_rebuild = false);
	void BuildMeshSDF();
	void BuildMeshCards();
};