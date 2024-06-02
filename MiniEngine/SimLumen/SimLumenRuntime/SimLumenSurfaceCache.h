#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"

class CSimLumenSurfaceCache
{
public:
	void Init();
	void SurfaceCacheDirectLighting(ComputeContext& cptContext);
	void SurfaceCacheInjectLighting(ComputeContext& cptContext);
	void SurfaceCacheCombineLighting(ComputeContext& cptContext);
private:
	RootSignature m_surface_cache_direct_light_sig;
	ComputePSO m_surface_cache_direct_light_pso;

	RootSignature m_scache_combine_light_sig;
	ComputePSO m_scache_combine_light_pso;

	RootSignature m_scache_inject_light_sig;
	ComputePSO m_scache_inject_light_pso;
};