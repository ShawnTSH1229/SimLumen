#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"
class CSimLumenShadow
{
public:
	void Init();
	void RenderingShadowMap(GraphicsContext& gfxContext);
private:
	ShadowCamera m_SunShadow;

	RootSignature m_shadow_rendering_sig;
	GraphicsPSO m_shadow_rendering_pso;
};
