#include "pch.h"
#include "GameCore.h"
#include "GraphicsCore.h"
#include "SystemTime.h"
#include "TextRenderer.h"
#include "GameInput.h"
#include "CommandContext.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "BufferManager.h"

#include <memory>
#include "SimLumenCommon/ShaderCompile.h"
#include "SimLumenMeshBuilder/SimLumenMeshBuilder.h"
#include "SimLumenRuntime/SimLumenMeshInstance.h"
#include "SimLumenRuntime/SimLumenCardCapture.h"
#include "SimLumenRuntime/SimLumenGlobalResource.h"
#include "SimLumenRuntime/SimLumenVisualization.h"
#include "SimLumenRuntime/SimLumenVoxelScene.h"
#include "SimLumenRuntime/SimLumenShadow.h"
#include "SimLumenRuntime/SimLumenGBufferGeneration.h"
#include "SimLumenRuntime/SimLumenLightingPass.h"
#include "SimLumenRuntime/SimLumenSurfaceCache.h"
#include "SimLumenRuntime/SimLumenRadiosity.h"
#include "SimLumenRuntime/SimLumenFinalGather.h"

using namespace GameCore;
using namespace Graphics;

class SimLumen : public GameCore::IGameApp
{
public:

    SimLumen()
    {
    }

    virtual void Startup( void ) override;
    virtual void Cleanup( void ) override;

    virtual void Update( float deltaT ) override;
    virtual void RenderScene( void ) override;

    void UpdateConstantBuffer(GraphicsContext& cbUpdateContext);
private:

    Camera m_Camera;
    std::unique_ptr<CameraController> m_CameraController;

    CSimLumenShadow  m_shadowpass;
    CSimLumenMeshBuilder m_MeshBuilder;
    CSimLuCardCapturer m_card_capturer;
    CSimLumenVisualization m_lumen_visualizer;
    CSimLumenVoxelScene m_lumen_vox_scene;
    CSimLumenGBufferGeneration m_gbuffer_generation;
    CSimLumenLightingPass m_lighting_pass;
    CSimLumenSurfaceCache m_lumen_surface_cache;
    CSimLumenRadiosity m_radiosity_pass;
    CSimLumenFinalGather m_final_gather;

    Math::Matrix4 pre_view_matrix;
};

CREATE_APPLICATION( SimLumen )

void SimLumen::Startup( void )
{
    MotionBlur::Enable = false;
    TemporalEffects::EnableTAA = false;
    FXAA::Enable = false;
    PostEffects::EnableHDR = false;
    PostEffects::BloomEnable = false;
    PostEffects::EnableAdaptation = false;
    SSAO::Enable = false;

    GetGlobalResource().m_shader_compiler.Init();


    std::vector<SLumenMeshInstance>& scene_mesh = GetGlobalResource().m_mesh_instances;
    CreateDemoScene(scene_mesh);

    std::vector<CSimLumenMeshResouce*> meshs;
    meshs.resize(scene_mesh.size());
    for (int mesh_idx = 0; mesh_idx < scene_mesh.size(); mesh_idx++)
    {
        meshs[mesh_idx] = &scene_mesh[mesh_idx].m_mesh_resource;
    }
    BuildGlobalSDFData(meshs, GetGlobalResource().global_sdf_data, false);


    m_MeshBuilder.Init();
    for (int mesh_idx = 0; mesh_idx < scene_mesh.size(); mesh_idx++)
    {
        m_MeshBuilder.BuildMesh(scene_mesh[mesh_idx].m_mesh_resource);
    }
    m_MeshBuilder.Destroy();

    m_Camera.SetEyeAtUp(Vector3(0, 50, 100), Vector3(kZero), Vector3(kYUnitVector));
    m_Camera.SetZRange(1.0f, 10000.0f);
    m_CameraController.reset(new FlyingFPSCamera(m_Camera, Vector3(kYUnitVector)));
    m_card_capturer.Init();
    m_lumen_vox_scene.Init();
    InitGlobalResource();

    m_lumen_visualizer.Init();
    m_shadowpass.Init();
    m_gbuffer_generation.Init();
    m_lighting_pass.Init();
    m_lumen_surface_cache.Init();
    m_radiosity_pass.Init();
    m_final_gather.Init();
}

namespace Graphics
{
    extern EnumVar DebugZoom;
}

void SimLumen::Cleanup( void )
{
    // Free up resources in an orderly fashion
}

#define VIS_KEY_PRESS(x)\
if (GameInput::IsFirstPressed(GameInput::kKey_##x)) { GetGlobalResource().m_visualize_type = x; };

#define VIS_KEY_PRESS_NUM(x,y)\
if (GameInput::IsFirstPressed(GameInput::kKey_##x)) { GetGlobalResource().m_visualize_type = y; };

void SimLumen::Update( float deltaT )
{
    ScopedTimer _prof(L"Update State");

    if (GameInput::IsFirstPressed(GameInput::kLShoulder))
        DebugZoom.Decrement();
    else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
        DebugZoom.Increment();

    m_CameraController->Update(deltaT);

    VIS_KEY_PRESS(0);
    VIS_KEY_PRESS(1);
    VIS_KEY_PRESS(2);
    VIS_KEY_PRESS(3);
    VIS_KEY_PRESS(4);
    VIS_KEY_PRESS(5);
    VIS_KEY_PRESS(6);
    VIS_KEY_PRESS(7);
    VIS_KEY_PRESS(8);
    VIS_KEY_PRESS(9);

    VIS_KEY_PRESS_NUM(t, 10);
    VIS_KEY_PRESS_NUM(y, 11);
    VIS_KEY_PRESS_NUM(u, 12);
    VIS_KEY_PRESS_NUM(i, 13);
    VIS_KEY_PRESS_NUM(o, 14);
    VIS_KEY_PRESS_NUM(p, 15);
    VIS_KEY_PRESS_NUM(g, 16);

    if (GameInput::IsFirstPressed(GameInput::kKey_m))
    {
        GetGlobalResource().m_disable_lighd_is = !GetGlobalResource().m_disable_lighd_is;
    }
    GetGlobalResource().m_lumen_scene_info.frame_num++;
}

void SimLumen::RenderScene( void )
{
    {
        GraphicsContext& gfxContext = GraphicsContext::Begin(L"PreProcess");
        UpdateConstantBuffer(gfxContext);
        m_card_capturer.UpdateSceneCards(gfxContext);
        m_shadowpass.RenderingShadowMap(gfxContext);
        m_gbuffer_generation.Rendering(gfxContext);
        gfxContext.Finish();
    }
    
   {
       ComputeContext& cptContext = ComputeContext::Begin(L"Lumen Scene Updata");
       m_lumen_vox_scene.UpdateVisibilityBuffer(cptContext);
       m_lumen_surface_cache.SurfaceCacheDirectLighting(cptContext);
       m_lumen_surface_cache.SurfaceCacheCombineLighting(cptContext);
       m_lumen_surface_cache.SurfaceCacheInjectLighting(cptContext);
       m_radiosity_pass.RadiosityTrace(cptContext);
       m_final_gather.Rendering(cptContext);
       cptContext.Finish();
   }

    {
        GraphicsContext& gfxContext = GraphicsContext::Begin(L"Lighting");
        m_lighting_pass.Rendering(gfxContext);
        m_lumen_visualizer.Render(gfxContext);
        gfxContext.Finish();
    }
}

void SimLumen::UpdateConstantBuffer(GraphicsContext& cbUpdateContext)
{
    SLumenViewGlobalConstant globals;
    globals.ViewProjMatrix = m_Camera.GetViewProjMatrix();
    globals.CameraPos = m_Camera.GetPosition();
    globals.SunDirection = GetGlobalResource().m_LightDirection;
    globals.SunIntensity = Math::Vector3(1, 1, 1);
    globals.ShadowViewProjMatrix = GetGlobalResource().m_shadow_vpmatrix;
    globals.InverseViewProjMatrix = Math::Invert(globals.ViewProjMatrix);
    globals.PointLightWorldPos = DirectX::XMFLOAT3(-20, 32, -50);
    globals.PointLightRadius = 20.0f;
    globals.PreViewProjMatrix = pre_view_matrix;
    globals.VisualizeType = GetGlobalResource().m_visualize_type;
    DynAlloc cb = cbUpdateContext.ReserveUploadMemory(sizeof(SLumenViewGlobalConstant));
    memcpy(cb.DataPtr, &globals, sizeof(SLumenViewGlobalConstant));
    GetGlobalResource().m_global_view_constant_buffer = cb.GpuAddress;
    pre_view_matrix = globals.ViewProjMatrix;

    SMeshSdfBrickTextureInfo global_sdf_info;
    global_sdf_info.texture_brick_num_x = SDF_BRICK_NUM_XY;
    global_sdf_info.texture_brick_num_y = SDF_BRICK_NUM_XY;

    global_sdf_info.texture_size_x = SDF_BRICK_TEX_SIZE;
    global_sdf_info.texture_size_y = SDF_BRICK_TEX_SIZE;
    global_sdf_info.texture_size_z = g_brick_size;

    global_sdf_info.scene_mesh_sdf_num = GetGlobalResource().m_mesh_instances.size();

    global_sdf_info.gloabl_sdf_voxel_size = gloabl_sdf_voxel_size;
    global_sdf_info.gloabl_sdf_center = gloabl_sdf_center;

    global_sdf_info.global_sdf_extents = gloabl_sdf_extent;
    global_sdf_info.global_sdf_scale_x = gloabl_sdf_scale_x;

    global_sdf_info.global_sdf_tex_size_xyz = DirectX::XMFLOAT3(global_sdf_size_x, global_sdf_size_y, global_sdf_size_z);
    global_sdf_info.global_sdf_scale_y = gloabl_sdf_scale_y;

    DynAlloc gloabl_sdf = cbUpdateContext.ReserveUploadMemory(sizeof(SMeshSdfBrickTextureInfo));
    memcpy(gloabl_sdf.DataPtr, &global_sdf_info, sizeof(SMeshSdfBrickTextureInfo));
    GetGlobalResource().m_mesh_sdf_brick_tex_info = gloabl_sdf.GpuAddress;
}
