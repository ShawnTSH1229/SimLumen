#pragma once
#include "SimLumenGlobalResource.h"

class CSimLumenGloablSDF
{
public:
	void Init();
	void Update();
private:
	RootSignature m_global_sdf_sig;
	GraphicsPSO m_global_sdf_pso;
};