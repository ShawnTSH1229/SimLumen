#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"

class CSimLumenGBufferGeneration
{
public:
	void Init();
	void Rendering(GraphicsContext& gfxContext);
private:
	RootSignature m_gbuffer_gen_sig;
	GraphicsPSO m_gbuffer_gen_pso;
};