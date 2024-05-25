#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"
class CSimLumenShadow
{
public:
	void Init();
private:
	ShadowCamera m_SunShadow;
};
