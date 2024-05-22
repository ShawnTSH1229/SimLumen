#include "MeshResource.h"
#include "Utility.h"
#include <fstream>

using namespace Graphics;
using namespace DirectX;


void CSimLumenMeshResouce::LoadFrom(const std::wstring& source_file, bool bforce_rebuild)
{
	const std::wstring obj_file_Name = source_file;
	const std::wstring lumen_file_name = Utility::RemoveExtension(source_file) + L".lumen";

	struct _stat64 sourceFileStat;
	struct _stat64 miniFileStat;

	bool obj_file_missing = _wstat64(obj_file_Name.c_str(), &sourceFileStat) == -1;
	bool lumen_file_missing = _wstat64(lumen_file_name.c_str(), &miniFileStat) == -1;

	if (obj_file_missing && lumen_file_missing)
	{
		Utility::Printf("Error: Could not find %ws\n", obj_file_Name.c_str());
		return;
	}

	bool needBuild = bforce_rebuild;
	if (lumen_file_missing)
	{
		needBuild = true;
	}

	if (!needBuild)
	{
		std::ifstream in_file = std::ifstream(lumen_file_name, std::ios::in | std::ios::binary);
		in_file.read((char*)&m_header, sizeof(SSimLumenResourceHeader));
		m_volume_df_data.distance_filed_volume.resize(m_header.volume_array_size);
		in_file.read((char*)m_volume_df_data.distance_filed_volume.data(), m_volume_df_data.distance_filed_volume.size() * sizeof(SDFBrickData));
		m_need_rebuild = false;
		m_need_save = false;
	}
	else
	{
		m_need_save = true;
		m_need_rebuild = true;
	}
	
}

void CSimLumenMeshResouce::SaveTo(const std::string& source_file_an)
{
	std::ofstream out_file(source_file_an, std::ios::out | std::ios::binary);
	if (!out_file)
	{
		return;
	}

	m_header.volume_array_size = m_volume_df_data.distance_filed_volume.size();
	out_file.write((char*)&m_header, sizeof(SSimLumenResourceHeader));
	out_file.write((char*)m_volume_df_data.distance_filed_volume.data(), m_volume_df_data.distance_filed_volume.size() * sizeof(SDFBrickData));
}

