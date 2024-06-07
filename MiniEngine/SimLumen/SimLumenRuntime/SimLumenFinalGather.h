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
	
	// importance sampling
	void InitBrdfPdfPso();
	void InitLightPdfPso();
	void InitStructuredISPso();

	// filter and gather
	void InitSSProbeTraceMeshSDFPso();
	void InitSSProbeTraceVoxelPso();
	void InitSSProbeCompositePso();
	void InitSSProbeFilterPso();
	void InitSSProbeConvertToOCTPso();
	void InitSSProbeIntegratePso();

	// importance sampling
	void LightingPdfSH(ComputeContext& cptContext);
	void LightingHistPdfSH(ComputeContext& cptContext);
	void BRDFPdfSH(ComputeContext& cptContext);
	void BuildStructuredIS(ComputeContext& cptContext);

	// filter and gather
	void SSProbeTraceMeshSDF(ComputeContext& cptContext);
	void SSProbeTraceVoxel(ComputeContext& cptContext);
	void SSProbeComposite(ComputeContext& cptContext);
	void SSProbeFilter(ComputeContext& cptContext);
	void SSProbeToOct(ComputeContext& cptContext);
	void SSProbeIntegrate(ComputeContext& cptContext);

	// importance sampling
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

	// filter and gather
	RootSignature m_ssprobe_trace_mesh_sdf_sig;
	ComputePSO m_ssprobe_trace_mesh_sdf_pso;

	RootSignature m_ssprobe_voxel_sig;
	ComputePSO m_ssprobe_voxel_pso;

	RootSignature m_ssprobe_composite_sig;
	ComputePSO m_ssprobe_composite_pso;

	RootSignature m_ssprobe_filter_sig;
	ComputePSO m_ssprobe_filter_pso;

	RootSignature m_ssprobe_to_oct_sig;
	ComputePSO m_ssprobe_to_oct_pso;

	RootSignature m_ssprobe_integrate_sig;
	ComputePSO m_ssprobe_integrate_pso;
};