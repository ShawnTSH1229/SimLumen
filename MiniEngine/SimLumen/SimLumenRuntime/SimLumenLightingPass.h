#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"

class CSimLumenLightingPass
{
public:
	void Init();
	void Rendering(GraphicsContext& gfxContext);
private:
	RootSignature m_lighting_sig;
	GraphicsPSO m_lighting_pso;
};