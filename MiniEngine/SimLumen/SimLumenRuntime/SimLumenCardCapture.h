#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshInstance.h"
#include "SimLumenGlobalResource.h"

struct STempCardBuffer
{
	ColorBuffer m_temp_card_albedo;
	ColorBuffer m_temp_card_normal;
	DepthBuffer m_temp_card_depth;
};

class CardCamera : public Math::BaseCamera
{
public:
	CardCamera() {}
	inline void SetProjMatrix(const Matrix4& ProjMat) { m_ProjMatrix = ProjMat; };
};

__declspec(align(256)) struct SCardCaptureConstant
{
	Math::Matrix4 ViewProjMatrix;
};

__declspec(align(256)) struct SCardCopyConstant
{
	// index [0 - 32)
	// scale 128 / 4096
	Math::Vector4 m_dest_atlas_index_and_scale;
};

class CSimLuCardCapturer
{
public:
	void Init();
	void UpdateSceneCards(GraphicsContext& gfxContext);
private:
	void CreatePSO();

	std::vector<STempCardBuffer> temp_cards;

	RootSignature m_card_capture_root_sig;
	GraphicsPSO m_card_capture_pso;

	RootSignature m_card_copy_root_sig;
	GraphicsPSO m_card_copy_pso;

	CardCamera m_card_camera;

	bool m_need_updata_cards;
	bool m_gen_cards;
};