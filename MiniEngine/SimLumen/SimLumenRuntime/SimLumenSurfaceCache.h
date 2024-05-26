#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"

class CSimLumenSurfaceCache
{
public:
	void Init();
	void Rendering();
private:
	RootSignature m_surface_cache_direct_light_sig;
	ComputePSO m_surface_cache_direct_light_pso;
};