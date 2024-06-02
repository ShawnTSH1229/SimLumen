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
	void InitLightPdfPso();
	void InitStructuredISPso();

	void LightingPdfSH(ComputeContext& cptContext);
	void BRDFPdfSH(ComputeContext& cptContext);
	void BuildStructuredIS(ComputeContext& cptContext);

	RootSignature m_brdf_pdf_sig;
	ComputePSO m_brdf_pdf_pso;
	RootSignature m_brdf_pdf_vis_sig;
	ComputePSO m_brdf_pdf_vis_pso;

	RootSignature m_light_pdf_sig;
	ComputePSO m_light_pdf_pso;
	RootSignature m_light_hist_pdf_sig;
	ComputePSO m_light_hist_pdf_pso;

	RootSignature m_sturct_is_sig;
	ComputePSO m_struct_is_pso;
};