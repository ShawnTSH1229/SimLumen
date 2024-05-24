#pragma once
#include "../SimLumenCommon/MeshResource.h"
#include "../embree/include/embree4/rtcore.h"

class CSimLumenMeshBuilder
{
public:
	void Init();
	void Destroy();

	// Create Mesh Bound
	void BuildMesh(CSimLumenMeshResouce& mesh);
private:
	void BuildMeshSDF(CSimLumenMeshResouce& mesh);
	void BuildMeshCard(CSimLumenMeshResouce& mesh);

	Math::BoundingBox ComputeMeshCard(XMFLOAT3 start_trace_pos, XMFLOAT3 end_trace_pos, XMFLOAT3 trace_dir, int dimension_x, int dimension_y, int dimension_z);

	RTCDevice m_rt_device;
	RTCScene m_rt_scene;
};

void BuildGlobalSDFData(std::vector<CSimLumenMeshResouce*> meshs, std::vector<uint8_t>& global_sdf_data, bool bforce_rebuild = false);

