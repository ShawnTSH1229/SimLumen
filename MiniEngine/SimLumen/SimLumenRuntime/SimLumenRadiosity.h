#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"

class CSimLumenRadiosity
{
public:
	void Init();
	void RadiosityTrace(ComputeContext& cptContext);
private:
	RootSignature m_radiosity_trace_sig;
	ComputePSO m_radiosity_trace_pso;

	RootSignature m_radiosity_filter_sig;
	ComputePSO m_radiosity_filter_pso;

	RootSignature m_radiosity_convertsh_sig;
	ComputePSO m_radiosity_convertsh_pso;

	RootSignature m_radiosity_integrate_sig;
	ComputePSO m_radiosity_integrate_pso;
};