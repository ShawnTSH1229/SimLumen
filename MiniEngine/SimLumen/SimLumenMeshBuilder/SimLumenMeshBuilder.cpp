#include "SimLumenMeshBuilder.h"

void CSimLumenMeshBuilder::Init()
{
	m_rt_device = rtcNewDevice(NULL);;
}

void CSimLumenMeshBuilder::Destroy()
{
	rtcReleaseDevice(m_rt_device);
}

void CSimLumenMeshBuilder::BuildMesh(CSimLumenMeshResouce& mesh)
{
	DirectX::XMFLOAT3 mesh_min_pos = DirectX::XMFLOAT3(1e30f, 1e30f, 1e30f);
	DirectX::XMFLOAT3 mesh_max_pos = DirectX::XMFLOAT3(-1e30f, -1e30f, -1e30f);
	for (DirectX::XMFLOAT3 pos : mesh.m_positions)
	{
		mesh_min_pos = DirectX::XMFLOAT3((std::min)(mesh_min_pos.x, pos.x), (std::min)(mesh_min_pos.y, pos.y), (std::min)(mesh_min_pos.z, pos.z));
		mesh_max_pos = DirectX::XMFLOAT3((std::max)(mesh_max_pos.x, pos.x), (std::max)(mesh_max_pos.y, pos.y), (std::max)(mesh_max_pos.z, pos.z));
	}

	DirectX::XMFLOAT3 center = DirectX::XMFLOAT3((mesh_min_pos.x + mesh_max_pos.x) * 0.5, (mesh_min_pos.y + mesh_max_pos.y) * 0.5, (mesh_min_pos.z + mesh_max_pos.z) * 0.5);
	DirectX::XMFLOAT3 extents = DirectX::XMFLOAT3(mesh_max_pos.x - center.x, mesh_max_pos.y - center.y, mesh_max_pos.z - center.z);
	mesh.m_BoundingBox = DirectX::BoundingBox(center, extents);

	m_rt_scene = rtcNewScene(m_rt_device);
	RTCGeometry geom = rtcNewGeometry(m_rt_device, RTC_GEOMETRY_TYPE_TRIANGLE);

	// upload daya
	float* vertices = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), mesh.m_positions.size());
	unsigned* indices = (unsigned*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned), mesh.m_indices.size() / 3);
	memcpy(vertices, mesh.m_positions.data(), mesh.m_positions.size() * sizeof(DirectX::XMFLOAT3));
	memcpy(indices, mesh.m_indices.data(), mesh.m_indices.size() * sizeof(unsigned));

	// create geometry
	rtcCommitGeometry(geom);
	rtcAttachGeometry(m_rt_scene, geom);
	rtcReleaseGeometry(geom);
	rtcCommitScene(m_rt_scene);

	BuildMeshCard(mesh);
	BuildMeshSDF(mesh);
	rtcReleaseScene(m_rt_scene);
}

XMFLOAT3 GetBouingBoxCorner(BoundingBox box, XMVECTORF32 corner)
{
	XMVECTOR vCenter = XMLoadFloat3(&box.Center);
	XMVECTOR vExtents = XMLoadFloat3(&box.Extents);

	XMFLOAT3 result;
	XMVECTOR C = XMVectorMultiplyAdd(vExtents, corner, vCenter);
	XMStoreFloat3(&result, C);
	return result;
}

void ComputeRotateBoundAndMatrix(const Math::BoundingBox& origin_box, Math::BoundingBox& cards_rotated_bound, Math::Matrix3& rotate_back, Vector3 Axis, const Scalar angle)
{
	origin_box.Transform(cards_rotated_bound, 1.0, Math::Quaternion(), -Math::Vector3(origin_box.Center));
	cards_rotated_bound.Transform(cards_rotated_bound, 1.0, Math::Quaternion(Axis, -angle), Math::Vector3(0, 0, 0));
	rotate_back = Math::Matrix3(Math::Quaternion(Axis, angle));
}

void CSimLumenMeshBuilder::BuildMeshCard(CSimLumenMeshResouce& mesh)
{
	XMFLOAT3 corners[8];
	mesh.m_BoundingBox.GetCorners(corners);

	Math::BoundingBox cards_bounds[6];
	Math::BoundingBox cards_rotated_bounds[6];
	Math::Matrix3 rotate_back[6];
	// x y plane
	cards_bounds[0] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,-1,+1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,+1,+1,0 }), XMFLOAT3(0, 0, -1), 0, 1, 2);
	cards_bounds[1] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,-1,-1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,+1,-1,0 }), XMFLOAT3(0, 0, +1), 0, 1, 2);

	ComputeRotateBoundAndMatrix(cards_bounds[0], cards_rotated_bounds[0], rotate_back[0], Vector3(1, 1, 1), 0);
	ComputeRotateBoundAndMatrix(cards_bounds[1], cards_rotated_bounds[1], rotate_back[1], Vector3(0, 1, 0), Math::XM_PI);

	// x z plane
	cards_bounds[2] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,+1,+1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,+1,-1,0 }), XMFLOAT3(0, -1, 0), 0, 2, 1);
	cards_bounds[3] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,-1,+1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,-1,-1,0 }), XMFLOAT3(0, +1, 0), 0, 2, 1);

	ComputeRotateBoundAndMatrix(cards_bounds[2], cards_rotated_bounds[2], rotate_back[2], Vector3(1, 0, 0), -Math::XM_PIDIV2);
	ComputeRotateBoundAndMatrix(cards_bounds[3], cards_rotated_bounds[3], rotate_back[3], Vector3(1, 0, 0), Math::XM_PIDIV2);
	
	// y z plane
	cards_bounds[4] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,-1,-1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,+1,+1,0 }), XMFLOAT3(-1, 0, 0), 1, 2, 0);
	cards_bounds[5] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,-1,-1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,+1,+1,0 }), XMFLOAT3(+1, 0, 0), 1, 2, 0);

	ComputeRotateBoundAndMatrix(cards_bounds[4], cards_rotated_bounds[4], rotate_back[4], Vector3(0, 1, 0), Math::XM_PIDIV2);
	ComputeRotateBoundAndMatrix(cards_bounds[5], cards_rotated_bounds[5], rotate_back[5], Vector3(0, 1, 0), -Math::XM_PIDIV2);

	for (int idx = 0; idx < 6; idx++)
	{
		mesh.m_cards[idx].m_local_boundbox = cards_bounds[idx];
		mesh.m_cards[idx].m_rotated_extents = cards_rotated_bounds[idx].Extents;
		mesh.m_cards[idx].m_bound_center = cards_bounds[idx].Center;
		mesh.m_cards[idx].m_rotate_back_matrix = rotate_back[idx];
	}
}



// palne : left top + right top + left bottom + right bottom;
Math::BoundingBox CSimLumenMeshBuilder::ComputeMeshCard(XMFLOAT3 palne_start_trace_pos, XMFLOAT3 plane_end_trace_pos, XMFLOAT3 trace_dir, int dimension_x, int dimension_y, int dimension_z)
{
	float x_stride = Math::Abs(GetFloatComponent(plane_end_trace_pos, dimension_x) - GetFloatComponent(palne_start_trace_pos, dimension_x)) / 128;
	float y_stride = Math::Abs(GetFloatComponent(plane_end_trace_pos, dimension_y) - GetFloatComponent(palne_start_trace_pos, dimension_y)) / 128;

	RTCIntersectArguments args;
	rtcInitIntersectArguments(&args);
	args.feature_mask = RTC_FEATURE_FLAG_NONE;

	float max_depth = 2.0;

	for (int x_idx = 0; x_idx < 128; x_idx++)
	{
		for (int y_idx = 0; y_idx < 128; y_idx++)
		{
			XMFLOAT3 trace_position = palne_start_trace_pos;
			AddFloatComponent(trace_position, dimension_x, (x_idx + 0.5) * x_stride);
			AddFloatComponent(trace_position, dimension_y, (y_idx + 0.5) * y_stride);

			RTCRayHit embree_ray;
			embree_ray.ray.org_x = trace_position.x;
			embree_ray.ray.org_y = trace_position.y;
			embree_ray.ray.org_z = trace_position.z;
			embree_ray.ray.dir_x = trace_dir.x;
			embree_ray.ray.dir_y = trace_dir.y;
			embree_ray.ray.dir_z = trace_dir.z;
			embree_ray.ray.tnear = 0;
			embree_ray.ray.tfar = 1e30f;
			embree_ray.ray.time = 0;
			embree_ray.ray.mask = -1;
			embree_ray.hit.u = embree_ray.hit.v = 0;
			embree_ray.hit.geomID = RTC_INVALID_GEOMETRY_ID;
			embree_ray.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
			embree_ray.hit.primID = RTC_INVALID_GEOMETRY_ID;

			rtcIntersect1(m_rt_scene, &embree_ray, &args);

			if ((embree_ray.ray.tfar != 1e30f) && embree_ray.hit.geomID != RTC_INVALID_GEOMETRY_ID && embree_ray.hit.primID != RTC_INVALID_GEOMETRY_ID)
			{
				Math::Vector3 hit_normal(embree_ray.hit.Ng_x, embree_ray.hit.Ng_y, embree_ray.hit.Ng_z);
				hit_normal = Math::Normalize(hit_normal);
				float dot_value = Math::Dot(trace_dir, hit_normal);

				if (dot_value < 0 && max_depth < embree_ray.ray.tfar)
				{
					max_depth = embree_ray.ray.tfar;
				}
			}
		}
	}

	XMFLOAT3 points[4];
	points[0] = palne_start_trace_pos;
	points[1] = plane_end_trace_pos;
	
	points[2] = palne_start_trace_pos;
	points[2].x += trace_dir.x * max_depth;
	points[2].y += trace_dir.y * max_depth;
	points[2].z += trace_dir.z * max_depth;

	points[3] = plane_end_trace_pos;
	points[3].x += trace_dir.x * max_depth;
	points[3].y += trace_dir.y * max_depth;
	points[3].z += trace_dir.z * max_depth;

	Math::BoundingBox RetBound;
	Math::BoundingBox::CreateFromPoints(RetBound, 4, points, sizeof(XMFLOAT3));
	return RetBound;
}
