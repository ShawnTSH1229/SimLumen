#pragma once
#include "SimLumenGlobalResource.h"

class CSimLumenVisualization
{
public:
	void Init();
	void Render(GraphicsContext& gfxContext);
private:
	void VisualizeMeshSDFs(GraphicsContext& gfxContext);
	void VisualizeGloablSDFs(GraphicsContext& gfxContext);
	void VisualizeSurfaceCache(GraphicsContext& gfxContext);

	void InitSurfaceCachePSO();
	void InitSDFVisPSO();
	void InitSDFVisBuffer();
	void InitVisVoxelLightBuffer();
	void InitSurfaceCacheVisBuffer();

	ByteAddressBuffer m_sdf_vis_pos_buffer;
	ByteAddressBuffer m_sdf_vis_direction_buffer;
	ByteAddressBuffer m_sdf_vis_index_buffer;
	StructuredBuffer m_sdf_instance_buffer;
	
	int m_sdf_vis_index_count_perins;
	int m_instance_num;

	// visualize mesh sdf
	RootSignature m_vis_sdf_sig;
	GraphicsPSO m_vis_sdf_pso;

	// visualize global sdf
	RootSignature m_vis_global_sdf_sig;
	GraphicsPSO m_vis_global_sdf_pso;

	//  visualize surface cache
	ByteAddressBuffer m_scache_vis_pos_buffer;
	ByteAddressBuffer m_scache_vis_uv_buffer;
	ByteAddressBuffer m_scache_vis_index_buffer;
	RootSignature m_vis_scache_sig;
	GraphicsPSO m_vis_scache_pso;

	// voxel lighting visualization
	ByteAddressBuffer m_vox_vis_pos_buffer;
	ByteAddressBuffer m_vox_vis_direction_buffer;
	ByteAddressBuffer m_vox_vis_index_buffer;

	int m_vox_vis_index_count_perins;

	RootSignature m_vis_voxlight_sig;
	GraphicsPSO m_vis_voxlight_pso;
};