#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"

class CSimLumenRadiosity
{
public:
	void Init();
	void RadiosityTrace();
private:
	RootSignature m_radiosity_trace_sig;
	ComputePSO m_radiosity_trace_pso;
};