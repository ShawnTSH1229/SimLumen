#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"
#include "../SimLumenCommon/MeshResource.h"

#include "TextureConvert.h"
#include "GameCore.h"
#include "CameraController.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CommandContext.h"
#include "TemporalEffects.h"
#include "MotionBlur.h"
#include "DepthOfField.h"
#include "PostEffects.h"
#include "SSAO.h"
#include "FXAA.h"
#include "SystemTime.h"
#include "TextRenderer.h"
#include "ParticleEffectManager.h"
#include "GameInput.h"
#include "glTF.h"
#include "Renderer.h"
#include "Model.h"
#include "ModelLoader.h"
#include "ShadowCamera.h"
#include "Display.h"

__declspec(align(256)) struct SLumenViewGlobalConstant
{
	Math::Matrix4 ViewProjMatrix;
	Math::Vector3 CameraPos;
	Math::Vector3 SunDirection;
	Math::Vector3 SunIntensity;
	Math::Matrix4 ShadowViewProjMatrix;
	Math::Matrix4 InverseViewProjMatrix;
};

__declspec(align(256)) struct SLumenMeshConstant
{
	Math::Matrix4 WorldMatrix;
	Math::Matrix3 WorldIT;
	Math::Vector4 ColorMulti = DirectX::XMFLOAT4(1, 1, 1, 1);
};

struct SLumenMeshInstance
{
	CSimLumenMeshResouce m_mesh_resource;

	ByteAddressBuffer m_vertex_pos_buffer;
	ByteAddressBuffer m_vertex_norm_buffer;
	ByteAddressBuffer m_vertex_uv_buffer;
	ByteAddressBuffer m_index_buffer;

	SLumenMeshConstant m_LumenConstant;

	TextureRef m_tex;
};

void CreateDemoScene(std::vector<SLumenMeshInstance>& out_mesh_instances);