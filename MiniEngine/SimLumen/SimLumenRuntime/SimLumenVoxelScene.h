#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"

class CSimLumenVoxelScene
{
public:
	void Init();
	void UpdateVisibilityBuffer(ComputeContext& cptContext);

private:

	RootSignature m_vox_vis_update_sig;
	ComputePSO m_vox_vis_update_pso;
	bool need_update_vis_buffer;
};