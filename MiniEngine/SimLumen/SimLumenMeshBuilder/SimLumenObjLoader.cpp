#include "SimLumenObjLoader.h"
#include <fstream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

bool LoadObj(const std::string& inputfile, CSimLumenMeshResouce* out_mesh_data)
{
    if (!out_mesh_data)
    {
        return false;
    }

    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(inputfile, reader_config)) 
    {
        if (!reader.Error().empty()) 
        {
            return false;
        }
    }


    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    int total_index_num = 0;
    for (size_t s = 0; s < shapes.size(); s++)
    {
        total_index_num += shapes[s].mesh.indices.size();
    }

    int total_vtx_num = 0;

    for (size_t s = 0; s < shapes.size(); s++)
    {
        total_vtx_num += attrib.vertices.size() / 3;
    }

    out_mesh_data->m_indices.resize(total_index_num);
    out_mesh_data->m_positions.resize(total_vtx_num);
    out_mesh_data->m_normals.resize(total_vtx_num);
    out_mesh_data->m_uvs.resize(total_vtx_num);

    int index_offset = 0;
    for (size_t s = 0; s < shapes.size(); s++)
    {
        for (int indice_idx = 0; indice_idx < shapes[s].mesh.indices.size(); indice_idx ++)
        {
            tinyobj::index_t idx = shapes[s].mesh.indices[indice_idx];

            tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
            tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
            tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

            int vtx_indx = idx.vertex_index;

            if (idx.normal_index >= 0)
            {
                tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                out_mesh_data->m_normals[vtx_indx] = DirectX::XMFLOAT3(nx, ny, nz);
            }
            else
            {
                out_mesh_data->m_normals[vtx_indx] = DirectX::XMFLOAT3(0, 0, 1);
            }

            if (idx.texcoord_index >= 0)
            {
                tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                out_mesh_data->m_uvs[vtx_indx] = DirectX::XMFLOAT2(tx, ty);
            }

            out_mesh_data->m_positions[vtx_indx] = DirectX::XMFLOAT3(vx, vy, vz);
            out_mesh_data->m_indices[indice_idx + index_offset] = vtx_indx;
        }
        index_offset += shapes[s].mesh.indices.size();
    }
    return true;
}