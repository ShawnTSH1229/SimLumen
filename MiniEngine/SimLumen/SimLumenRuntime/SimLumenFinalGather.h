#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"

class CSimLumenFinalGather
{
public:
	void Init();
	void Rendering(ComputeContext& cptContext);
private:
	void InitBrdfPdfPso();

	void BRDFPdfSH(ComputeContext& cptContext);

	RootSignature m_brdf_pdf_sig;
	ComputePSO m_brdf_pdf_pso;

	RootSignature m_brdf_pdf_vis_sig;
	ComputePSO m_brdf_pdf_vis_pso;
};