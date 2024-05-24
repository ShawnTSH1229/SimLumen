#include "SimLumenMeshBuilder.h"
#include <fstream>

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
void CSimLumenMeshBuilder::BuildMeshSDF(CSimLumenMeshResouce& mesh)
{
	SVolumeDFBuildData& volumeData = mesh.m_volume_df_data;
	int brick_volume_size = g_brick_size * volumeData.m_VoxelSize;
	int half_brick_volume_size = brick_volume_size / 2;

	assert(brick_volume_size == half_brick_volume_size * 2);

	XMINT3 center_int(mesh.m_BoundingBox.Center.x, mesh.m_BoundingBox.Center.y, mesh.m_BoundingBox.Center.z);
	XMINT3 extent_int(mesh.m_BoundingBox.Extents.x + 0.5f, mesh.m_BoundingBox.Extents.y + 0.5f, mesh.m_BoundingBox.Extents.z + 0.5f);

	extent_int.x = (extent_int.x + half_brick_volume_size - 1) / half_brick_volume_size;
	extent_int.y = (extent_int.y + half_brick_volume_size - 1) / half_brick_volume_size;
	extent_int.z = (extent_int.z + half_brick_volume_size - 1) / half_brick_volume_size;

	extent_int.x *= half_brick_volume_size;
	extent_int.y *= half_brick_volume_size;
	extent_int.z *= half_brick_volume_size;

	XMINT3 volume_min_pos(center_int.x - extent_int.x, center_int.y - extent_int.y, center_int.z - extent_int.z);
	XMINT3 volume_max_pos(center_int.x + extent_int.x, center_int.y + extent_int.y, center_int.z + extent_int.z);

	volumeData.m_LocalSpaceBox.volume_min_pos = volume_min_pos;
	volumeData.m_LocalSpaceBox.volume_max_pos = volume_max_pos;

	std::vector<Math::Vector3> samples0;
	std::vector<Math::Vector3> samples1;
	GenerateStratifiedUniformHemisphereSamples(128, samples0);
	GenerateStratifiedUniformHemisphereSamples(128, samples1);

	for (int idx = 0; idx < samples1.size(); idx++)
	{
		samples0.push_back(samples1[idx]);
	}

	DirectX::BoundingBox bbox_uint(DirectX::XMFLOAT3(center_int.x, center_int.y, center_int.z), DirectX::XMFLOAT3(extent_int.x, extent_int.y, extent_int.z));

	int volume_brick_num_x = (volume_max_pos.x - volume_min_pos.x) / brick_volume_size;
	int volume_brick_num_y = (volume_max_pos.y - volume_min_pos.y) / brick_volume_size;
	int volume_brick_num_z = (volume_max_pos.z - volume_min_pos.z) / brick_volume_size;

	volumeData.distance_filed_volume.resize(volume_brick_num_x * volume_brick_num_y * volume_brick_num_z);
	volumeData.volume_brick_num_xyz = DirectX::XMUINT3(volume_brick_num_x, volume_brick_num_y, volume_brick_num_z);

	float radius_sqaure = extent_int.x * extent_int.x + extent_int.y * extent_int.y + extent_int.z * extent_int.z;
	float max_distance = Math::Sqrt(radius_sqaure) * 0.8;
	volumeData.m_max_distance = max_distance;
	// brick 0,0,0 | 1,0,0 | 0,1,0 | 1,1,0 | 0,0,1 | 1,0,1 | 0,1,1 | 1,1,1 | 

	if (mesh.m_need_rebuild)
	{
		for (int brick_index_z = 0; brick_index_z < volume_brick_num_z; brick_index_z++)
		{
			for (int brick_index_y = 0; brick_index_y < volume_brick_num_y; brick_index_y++)
			{
				for (int brick_index_x = 0; brick_index_x < volume_brick_num_x; brick_index_x++)
				{
					for (int brick_vol_idx_z = 0; brick_vol_idx_z < g_brick_size; brick_vol_idx_z++)
					{
						for (int brick_vol_idx_y = 0; brick_vol_idx_y < g_brick_size; brick_vol_idx_y++)
						{
							for (int brick_vol_idx_x = 0; brick_vol_idx_x < g_brick_size; brick_vol_idx_x++)
							{
								int hit_num = 0;
								int hit_back_num = 0;
								float min_distance = 1e30f;

								int vol_idx_x = brick_index_x * g_brick_size + brick_vol_idx_x;
								int vol_idx_y = brick_index_y * g_brick_size + brick_vol_idx_y;
								int vol_idx_z = brick_index_z * g_brick_size + brick_vol_idx_z;

								for (int smp_idx = 0; smp_idx < samples0.size(); smp_idx++)
								{
									Math::Vector3 sample_direction = samples0[smp_idx];
									Math::Vector3 start_position(
										volume_min_pos.x + vol_idx_x * volumeData.m_VoxelSize + volumeData.m_VoxelSize * 0.5,
										volume_min_pos.y + vol_idx_y * volumeData.m_VoxelSize + volumeData.m_VoxelSize * 0.5,
										volume_min_pos.z + vol_idx_z * volumeData.m_VoxelSize + volumeData.m_VoxelSize * 0.5);

									float ray_box_dist;
									if ((!(float(sample_direction.GetX()) == 0.0 && float(sample_direction.GetY()) == 0.0 && float(sample_direction.GetZ()) == 0.0)) &&
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

											float current_distant = embree_ray.ray.tfar;
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

								float scaled_min_distance = min_distance / max_distance;// -1->1
								float clamed_min_distance = Math::Clamp(scaled_min_distance * 0.5 + 0.5, 0, 1);//0 - 1
								uint8_t normalized_min_distance = uint8_t(int32_t(clamed_min_distance * 255.0f + 0.5));

								int brick_index = brick_index_z * volume_brick_num_y * volume_brick_num_x + brick_index_y * volume_brick_num_x + brick_index_x;
								volumeData.distance_filed_volume[brick_index].m_brick_data[brick_vol_idx_x][brick_vol_idx_y][brick_vol_idx_z] = normalized_min_distance;
							}
						}
					}
				}
			}
		}
	}
	
	if (mesh.m_need_save)
	{
		mesh.SaveTo(std::string("Assets/erato.lumen"));
	}
}

void BuildGlobalSDFData(std::vector<CSimLumenMeshResouce*> meshs, std::vector<uint8_t>& global_sdf_data, bool bforce_rebuild)
{
	struct _stat64 miniFileStat;
	const std::wstring lumen_file_name(L"Assets/global_sdf.lumen");
	const std::string lumen_file_name_u8("Assets/global_sdf.lumen");
	bool lumen_file_missing = _wstat64(lumen_file_name.c_str(), &miniFileStat) == -1;

	
	bool needBuild = bforce_rebuild;
	if (lumen_file_missing)
	{
		needBuild = true;
	}

	if (needBuild)
	{
		RTCDevice m_rt_device = rtcNewDevice(NULL);
		RTCScene m_rt_scene = rtcNewScene(m_rt_device);

		std::vector<RTCGeometry> scene_geos;
		scene_geos.resize(meshs.size());

		for (uint32_t mesh_idx = 0; mesh_idx < meshs.size(); mesh_idx++)
		{
			scene_geos[mesh_idx] = rtcNewGeometry(m_rt_device, RTC_GEOMETRY_TYPE_TRIANGLE);

			RTCGeometry geom = scene_geos[mesh_idx];
			unsigned* indices = (unsigned*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned), meshs[mesh_idx]->m_indices.size() / 3);
			memcpy(indices, meshs[mesh_idx]->m_indices.data(), meshs[mesh_idx]->m_indices.size() * sizeof(unsigned));

			float* vertices = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), meshs[mesh_idx]->m_positions.size());
			for (int vtx_idx = 0; vtx_idx < meshs[mesh_idx]->m_positions.size(); vtx_idx++)
			{
				Vector4 transformed_position = meshs[mesh_idx]->m_local_to_world * meshs[mesh_idx]->m_positions[vtx_idx];
				memcpy(((char*)vertices) + vtx_idx * sizeof(DirectX::XMFLOAT3), &transformed_position, sizeof(DirectX::XMFLOAT3));
			}

			rtcCommitGeometry(geom);
			rtcAttachGeometry(m_rt_scene, geom);
			rtcReleaseGeometry(geom);
		}
		rtcCommitScene(m_rt_scene);



		DirectX::XMFLOAT3 min_global_sdf_pos = DirectX::XMFLOAT3(gloabl_sdf_center.x - gloabl_sdf_extent.x, gloabl_sdf_center.y - gloabl_sdf_extent.y, gloabl_sdf_center.z - gloabl_sdf_extent.z);

		std::vector<Math::Vector3> samples0;
		std::vector<Math::Vector3> samples1;
		GenerateStratifiedUniformHemisphereSamples(128, samples0);
		GenerateStratifiedUniformHemisphereSamples(128, samples1);

		for (int idx = 0; idx < samples1.size(); idx++)
		{
			samples0.push_back(samples1[idx]);
		}

		DirectX::BoundingBox bbox(gloabl_sdf_center, gloabl_sdf_extent);

		global_sdf_data.resize(global_sdf_size_x * global_sdf_size_y * global_sdf_size_z);

		for (int vox_index_z = 0; vox_index_z < global_sdf_size_z; vox_index_z++)
		{
			for (int vox_index_y = 0; vox_index_y < global_sdf_size_y; vox_index_y++)
			{
				for (int vox_index_x = 0; vox_index_x < global_sdf_size_x; vox_index_x++)
				{
					int hit_num = 0;
					int hit_back_num = 0;
					float min_distance = 1e30f;

					for (int smp_idx = 0; smp_idx < samples0.size(); smp_idx++)
					{
						Math::Vector3 sample_direction = samples0[smp_idx];
						Math::Vector3 start_position(
							min_global_sdf_pos.x + vox_index_x + gloabl_sdf_voxel_size * 0.5,
							min_global_sdf_pos.y + vox_index_y + gloabl_sdf_voxel_size * 0.5,
							min_global_sdf_pos.z + vox_index_z + gloabl_sdf_voxel_size * 0.5);

						float ray_box_dist;
						if ((!(float(sample_direction.GetX()) == 0.0 && float(sample_direction.GetY()) == 0.0 && float(sample_direction.GetZ()) == 0.0)) && bbox.Intersects(start_position, sample_direction, ray_box_dist))
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

								float current_distant = embree_ray.ray.tfar;
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

					float scaled_min_distance = (min_distance - gloabl_sdf_scale_y) / gloabl_sdf_scale_x;// -1->1
					float clamed_min_distance = Math::Clamp(scaled_min_distance, 0, 1);//0 - 1
					uint8_t normalized_min_distance = uint8_t(int32_t(clamed_min_distance * 255.0f + 0.5));

					global_sdf_data[vox_index_z * global_sdf_size_y * global_sdf_size_x + vox_index_y * global_sdf_size_x + vox_index_x] = normalized_min_distance;
				}
			}
		}

		rtcReleaseScene(m_rt_scene);

		std::ofstream out_file(lumen_file_name_u8, std::ios::out | std::ios::binary);
		if (!out_file)
		{
			return;
		}

		out_file.write((char*)global_sdf_data.data(), global_sdf_data.size() * sizeof(uint8_t));
		out_file.close();
	}
	else
	{
		global_sdf_data.resize(global_sdf_size_x* global_sdf_size_y* global_sdf_size_z);
		std::ifstream in_file = std::ifstream(lumen_file_name, std::ios::in | std::ios::binary);
		in_file.read((char*) global_sdf_data.data(), global_sdf_data.size() * sizeof(SDFBrickData));
	}

	
}
