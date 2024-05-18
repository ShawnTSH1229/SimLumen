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

	void BuildSDF(CSimLumenMeshResouce& mesh);
	void BuildMeshCard(CSimLumenMeshResouce& mesh);

	Math::BoundingBox ComputeMeshCard(XMFLOAT3 start_trace_pos, XMFLOAT3 end_trace_pos, XMFLOAT3 trace_dir, int dimension_x, int dimension_y, int dimension_z);

	RTCDevice m_rt_device;
	RTCScene m_rt_scene;
};