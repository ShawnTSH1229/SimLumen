#include "SimLumenMeshBuilder.h"


#pragma optimize("",off)

static Math::Vector3 UniformSampleHemisphere(DirectX::XMFLOAT2 uniforms)
{
	uniforms.x = uniforms.x * 2.0 - 1.0;
	uniforms.y = uniforms.y * 2.0 - 1.0;

	if (uniforms.x == 0 && uniforms.y == 0)
	{
		return Math::Vector3(0, 0, 0);
	}

	float R;
	float Theta;

	if (Math::Abs(uniforms.x) > Math::Abs(uniforms.y))
	{
		R = uniforms.x;

		Theta = (float)Math::XM_PI / 4 * (uniforms.y / uniforms.x);
	}
	else
	{
		R = uniforms.y;
		Theta = (float)Math::XM_PI / 2 - (float)Math::XM_PI / 4 * (uniforms.x / uniforms.y);
	}

	const float U = R * Math::Cos(Theta);
	const float V = R * Math::Sin(Theta);
	const float R2 = R * R;

	return Math::Vector3(U * Math::Sqrt(2 - R2), V * Math::Sqrt(2 - R2), 1.0f - R2);
}

void GenerateStratifiedUniformHemisphereSamples(int m_sample_nums, std::vector<Math::Vector3>& samples)
{
	const int num_sample_dim = int(std::sqrt((float)m_sample_nums));

	samples.reserve(num_sample_dim * num_sample_dim);

	for (int idx_x = 0; idx_x < num_sample_dim; idx_x++)
	{
		for (int idx_y = 0; idx_y < num_sample_dim; idx_y++)
		{
			const float fraction1 = (idx_x) / (float)num_sample_dim;
			const float fraction2 = (idx_y) / (float)num_sample_dim;

			Math::Vector3 temp = UniformSampleHemisphere(DirectX::XMFLOAT2(fraction1, fraction2));
			samples.push_back(Math::Vector3(temp));
		}
	}
}

static constexpr int g_brick_size = 8;

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
	//BuildSDF(mesh);
	rtcReleaseScene(m_rt_scene);
}

void CSimLumenMeshBuilder::BuildSDF(CSimLumenMeshResouce& mesh)
{
	SVolumeDFBuildData& volumeData = mesh.m_volume_df_data;
	int brick_volume_size = g_brick_size * volumeData.m_VoxelSize;
	int half_brick_volume_size = brick_volume_size / 2;

	XMINT3 center_uint(mesh.m_BoundingBox.Center.x, mesh.m_BoundingBox.Center.y, mesh.m_BoundingBox.Center.z);
	XMINT3 extent_uint(mesh.m_BoundingBox.Extents.x + 0.5f, mesh.m_BoundingBox.Extents.y + 0.5f, mesh.m_BoundingBox.Extents.z + 0.5f);
	extent_uint.x = (extent_uint.x + half_brick_volume_size - 1) / half_brick_volume_size;
	extent_uint.y = (extent_uint.y + half_brick_volume_size - 1) / half_brick_volume_size;
	extent_uint.z = (extent_uint.z + half_brick_volume_size - 1) / half_brick_volume_size;

	extent_uint.x *= half_brick_volume_size;
	extent_uint.y *= half_brick_volume_size;
	extent_uint.z *= half_brick_volume_size;

	XMINT3 volume_min_pos(center_uint.x - extent_uint.x, center_uint.y - extent_uint.y, center_uint.z - extent_uint.z);
	XMINT3 volume_max_pos(center_uint.x + extent_uint.x, center_uint.y + extent_uint.y, center_uint.z + extent_uint.z);

	volumeData.m_LocalSpaceBox.volume_min_pos = volume_min_pos;
	volumeData.m_LocalSpaceBox.volume_max_pos = volume_min_pos;

	std::vector<Math::Vector3> samples0;
	std::vector<Math::Vector3> samples1;
	GenerateStratifiedUniformHemisphereSamples(128, samples0);
	GenerateStratifiedUniformHemisphereSamples(128, samples1);

	for (int idx = 0; idx < samples1.size(); idx++)
	{
		samples0.push_back(samples1[idx]);
	}

	DirectX::BoundingBox bbox_uint(DirectX::XMFLOAT3(center_uint.x, center_uint.y, center_uint.z), DirectX::XMFLOAT3(extent_uint.x, extent_uint.y, extent_uint.z));

	int volume_brick_num_x = (volume_max_pos.x - volume_min_pos.x) / brick_volume_size;
	int volume_brick_num_y = (volume_max_pos.y - volume_min_pos.y) / brick_volume_size;
	int volume_brick_num_z = (volume_max_pos.z - volume_min_pos.z) / brick_volume_size;

	int total_num = (volume_brick_num_x * volume_brick_num_y * volume_brick_num_z) * (g_brick_size * g_brick_size * g_brick_size);
	volumeData.distance_filed_volume.resize(total_num);

	float radius = extent_uint.x * extent_uint.x + extent_uint.y * extent_uint.y + extent_uint.z * extent_uint.z;
	radius = Math::Sqrt(radius) * 0.8;
	float max_distance = radius;

	//todo:!!!!set volume center to (0,0,0)

	for (int vol_idx_z = 0; vol_idx_z < volume_brick_num_z * g_brick_size; vol_idx_z++)
	{
		for (int vol_idx_y = 0; vol_idx_y < volume_brick_num_y * g_brick_size; vol_idx_y++)
		{
			for (int vol_idx_x = 0; vol_idx_x < volume_brick_num_x * g_brick_size; vol_idx_x++)
			{
				int store_idx = volume_brick_num_x * volume_brick_num_y * g_brick_size * g_brick_size * vol_idx_z + volume_brick_num_x * g_brick_size * vol_idx_y + vol_idx_x;

				int hit_num = 0;
				int hit_back_num = 0;
				float min_distance = 1e30f;

				for (int smp_idx = 0; smp_idx < samples0.size(); smp_idx++)
				{
					Math::Vector3 sample_direction = samples0[smp_idx];
					Math::Vector3 start_position(
						volume_min_pos.x + vol_idx_x * volumeData.m_VoxelSize + volumeData.m_VoxelSize * 0.5,
						volume_min_pos.y + vol_idx_y * volumeData.m_VoxelSize + volumeData.m_VoxelSize * 0.5,
						volume_min_pos.z + vol_idx_z * volumeData.m_VoxelSize + volumeData.m_VoxelSize * 0.5
					);

					start_position = start_position - 1e-3 * sample_direction;

					float ray_box_dist;
					if ((!(float(sample_direction.GetX()) == 0.0 &&
						float(sample_direction.GetY()) == 0.0 &&
						float(sample_direction.GetZ()) == 0.0)) &&
						bbox_uint.Intersects(start_position, sample_direction, ray_box_dist))
					{
						RTCIntersectArguments args;
						rtcInitIntersectArguments(&args);
						args.feature_mask = RTC_FEATURE_FLAG_NONE;

						RTCRayHit embree_ray;
						embree_ray.ray.org_x = start_position.GetX();
						embree_ray.ray.org_y = start_position.GetY();
						embree_ray.ray.org_z = start_position.GetZ();
						embree_ray.ray.dir_x = sample_direction.GetX();
						embree_ray.ray.dir_y = sample_direction.GetY();
						embree_ray.ray.dir_z = sample_direction.GetZ();
						embree_ray.ray.tnear = 0;
						embree_ray.ray.tfar = 1e30f;
						embree_ray.ray.time = 0;
						embree_ray.ray.mask = -1;
						embree_ray.hit.u = embree_ray.hit.v = 0;
						embree_ray.hit.geomID = RTC_INVALID_GEOMETRY_ID;
						embree_ray.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
						embree_ray.hit.primID = RTC_INVALID_GEOMETRY_ID;

						rtcIntersect1(m_rt_scene, &embree_ray, &args);

						if (embree_ray.hit.geomID != RTC_INVALID_GEOMETRY_ID && embree_ray.hit.primID != RTC_INVALID_GEOMETRY_ID)
						{
							hit_num++;

							Math::Vector3 hit_normal(embree_ray.hit.Ng_x, embree_ray.hit.Ng_y, embree_ray.hit.Ng_z);
							hit_normal = Math::Normalize(hit_normal);
							float dot_value = Math::Dot(sample_direction, hit_normal);

							if (dot_value > 0)
							{
								hit_back_num++;
							}

							const float current_distant = embree_ray.ray.tfar;

							if (current_distant < min_distance)
							{
								min_distance = current_distant;
							}
						}
					}
				}

				if (hit_num > 0 && hit_back_num > 0.25f * samples0.size())
				{
					min_distance *= -1;
				}

				min_distance = min_distance / max_distance;// -1->1
				min_distance = Math::Clamp(min_distance * 0.5 + 0.5, 0, 1);//0 - 1
				min_distance = uint8_t(int32_t(min_distance * 255.0f + 0.5));
				volumeData.distance_filed_volume[store_idx] = min_distance;
			}
		}
	}
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

void CSimLumenMeshBuilder::BuildMeshCard(CSimLumenMeshResouce& mesh)
{
	XMFLOAT3 corners[8];
	mesh.m_BoundingBox.GetCorners(corners);

	Math::BoundingBox cards_bounds[6];

	// x y plane
	cards_bounds[0] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,-1,+1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,+1,+1,0 }), XMFLOAT3(0, 0, -1), 0, 1, 2);
	cards_bounds[1] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,-1,-1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,+1,-1,0 }), XMFLOAT3(0, 0, +1), 0, 1, 2);

	// x z plane
	cards_bounds[2] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,+1,-1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,+1,+1,0 }), XMFLOAT3(0, -1, 0), 0, 2, 1);
	cards_bounds[3] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,-1,-1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,-1,+1,0 }), XMFLOAT3(0, +1, 0), 0, 2, 1);

	// y z plane
	cards_bounds[4] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,-1,-1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ +1,+1,+1,0 }), XMFLOAT3(-1, 0, 0), 1, 2, 0);
	cards_bounds[5] = ComputeMeshCard(GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,-1,-1,0 }), GetBouingBoxCorner(mesh.m_BoundingBox, XMVECTORF32{ -1,+1,+1,0 }), XMFLOAT3(+1, 0, 0), 1, 2, 0);

	for (int idx = 0; idx < 6; idx++)
	{
		mesh.m_cards[idx].m_local_boundbox = cards_bounds[idx];
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
