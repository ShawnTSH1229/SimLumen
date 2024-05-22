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
#include "SimLumenRuntime/SimLumenRender.h"
#include "SimLumenRuntime/SimLumenGlobalResource.h"
#include "SimLumenRuntime/SimLumenVisualization.h"

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

private:

    Camera m_Camera;
    std::unique_ptr<CameraController> m_CameraController;



    RootSignature m_BasePassRootSig;
   
    GraphicsPSO m_BasePassPSO;

    CSimLumenMeshBuilder m_MeshBuilder;
    CSimLuCardCapturer m_card_capturer;
    CSimLumenVisualization m_lumen_visualizer;
    
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

    GetGlobalResource().s_TextureHeap.Create(L"Scene Texture Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
    GetGlobalResource().s_SamplerHeap.Create(L"Scene Sampler Descriptors", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);

    GetGlobalResource().m_shader_compiler.Init();

    {

        m_BasePassRootSig.Reset(4, 0);
        m_BasePassRootSig[0].InitAsConstantBuffer(0);
        m_BasePassRootSig[1].InitAsConstantBuffer(1);
        m_BasePassRootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_BasePassRootSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_BasePassRootSig.Finalize(L"m_BasePassRootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    D3D12_INPUT_ELEMENT_DESC pos_norm_uv[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,      1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    DXGI_FORMAT ColorFormat = g_SceneColorBuffer.GetFormat();
    DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();

    // PSO
    {
        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/BasePass.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = GetGlobalResource().m_shader_compiler.Compile(L"Shaders/BasePass.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        m_BasePassPSO = GraphicsPSO(L"BasePass PSO");
        m_BasePassPSO.SetRootSignature(m_BasePassRootSig);
        m_BasePassPSO.SetRasterizerState(RasterizerDefault);
        m_BasePassPSO.SetBlendState(BlendDisable);
        m_BasePassPSO.SetDepthStencilState(DepthStateReadWrite);
        m_BasePassPSO.SetInputLayout(_countof(pos_norm_uv), pos_norm_uv);
        m_BasePassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_BasePassPSO.SetRenderTargetFormats(1, &ColorFormat, DepthFormat);
        m_BasePassPSO.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_BasePassPSO.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_BasePassPSO.Finalize();
    }

    std::vector<SLumenMeshInstance>& scene_mesh = GetGlobalResource().m_mesh_instances;
    CreateDemoScene(scene_mesh, GetGlobalResource().s_TextureHeap, GetGlobalResource().s_SamplerHeap);

    m_MeshBuilder.Init();
    for (int mesh_idx = 0; mesh_idx < scene_mesh.size(); mesh_idx++)
    {
        m_MeshBuilder.BuildMesh(scene_mesh[mesh_idx].m_mesh_resource);
    }
    m_MeshBuilder.Destroy();

    m_Camera.SetEyeAtUp(Vector3(0, 5, 5), Vector3(kZero), Vector3(kYUnitVector));
    m_Camera.SetZRange(1.0f, 10000.0f);
    m_CameraController.reset(new FlyingFPSCamera(m_Camera, Vector3(kYUnitVector)));

    SCardCaptureInitDesc card_capturer_init_desc;
    card_capturer_init_desc.m_shader_compiler = &GetGlobalResource().m_shader_compiler;
    m_card_capturer.Init(card_capturer_init_desc);

    InitGlobalResource();

    GetGlobalRender()->Init();
    m_lumen_visualizer.Init();
}

namespace Graphics
{
    extern EnumVar DebugZoom;
}

void SimLumen::Cleanup( void )
{
    // Free up resources in an orderly fashion
}

void SimLumen::Update( float deltaT )
{
    ScopedTimer _prof(L"Update State");

    if (GameInput::IsFirstPressed(GameInput::kLShoulder))
        DebugZoom.Decrement();
    else if (GameInput::IsFirstPressed(GameInput::kRShoulder))
        DebugZoom.Increment();

    m_CameraController->Update(deltaT);
}

void SimLumen::RenderScene( void )
{
    {
        GraphicsContext& gfxContext = GraphicsContext::Begin(L"update view constant buffer");
        
        SLumenViewGlobalConstant globals;
        globals.ViewProjMatrix = m_Camera.GetViewProjMatrix();
        globals.CameraPos = m_Camera.GetPosition();
        globals.SunDirection = GetLumenConfig().m_LightDirection;
        globals.SunIntensity = Math::Vector3(1, 1, 1);
        DynAlloc cb = gfxContext.ReserveUploadMemory(sizeof(SLumenViewGlobalConstant));
        memcpy(cb.DataPtr, &globals, sizeof(SLumenViewGlobalConstant));
        GetGlobalResource().m_global_view_constant_buffer = cb.GpuAddress;

        SMeshSdfBrickTextureInfo global_sdf_info;
        global_sdf_info.texture_brick_num_x = SDF_BRICK_NUM_XY;
        global_sdf_info.texture_brick_num_y = SDF_BRICK_NUM_XY;

        global_sdf_info.texture_size_x = SDF_BRICK_TEX_SIZE;
        global_sdf_info.texture_size_y = SDF_BRICK_TEX_SIZE;
        global_sdf_info.texture_size_z = g_brick_size;

        global_sdf_info.scene_mesh_sdf_num = GetGlobalResource().m_mesh_instances.size();
        DynAlloc gloabl_sdf = gfxContext.ReserveUploadMemory(sizeof(SMeshSdfBrickTextureInfo));
        memcpy(gloabl_sdf.DataPtr, &global_sdf_info, sizeof(SMeshSdfBrickTextureInfo));
        GetGlobalResource().m_mesh_sdf_brick_tex_info = gloabl_sdf.GpuAddress;

        gfxContext.Finish();
    }

    m_card_capturer.UpdateSceneCards(GetGlobalResource().m_mesh_instances, &GetGlobalResource().s_TextureHeap);

    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");



    gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
    gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    gfxContext.ClearDepth(g_SceneDepthBuffer);
    gfxContext.ClearColor(g_SceneColorBuffer);
    
    gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());
    gfxContext.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());

    gfxContext.SetRootSignature(m_BasePassRootSig);
    gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GetGlobalResource().s_TextureHeap.GetHeapPointer());
    gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GetGlobalResource().s_SamplerHeap.GetHeapPointer());
    gfxContext.SetPipelineState(m_BasePassPSO);
    gfxContext.SetConstantBuffer(1, GetGlobalResource().m_global_view_constant_buffer);
    gfxContext.FlushResourceBarriers();

    for (int mesh_idx = 0; mesh_idx < GetGlobalResource().m_mesh_instances.size(); mesh_idx++)
    {
        SLumenMeshInstance& lumen_mesh_instance = GetGlobalResource().m_mesh_instances[mesh_idx];

        DynAlloc mesh_consatnt = gfxContext.ReserveUploadMemory(sizeof(SLumenMeshConstant));
        memcpy(mesh_consatnt.DataPtr, &lumen_mesh_instance.m_LumenConstant, sizeof(SLumenMeshConstant));

        gfxContext.SetConstantBuffer(0, mesh_consatnt.GpuAddress);
        
        gfxContext.SetDescriptorTable(2, GetGlobalResource().s_TextureHeap[lumen_mesh_instance.m_tex_table_idx]);
        gfxContext.SetDescriptorTable(3, GetGlobalResource().s_SamplerHeap[lumen_mesh_instance.m_sampler_table_idx]);

        gfxContext.SetVertexBuffer(0, lumen_mesh_instance.m_vertex_pos_buffer.VertexBufferView());
        gfxContext.SetVertexBuffer(1, lumen_mesh_instance.m_vertex_norm_buffer.VertexBufferView());
        gfxContext.SetVertexBuffer(2, lumen_mesh_instance.m_vertex_uv_buffer.VertexBufferView());
        gfxContext.SetIndexBuffer(lumen_mesh_instance.m_index_buffer.IndexBufferView());
        
        gfxContext.DrawIndexed(lumen_mesh_instance.m_mesh_resource.m_indices.size());
    }

    gfxContext.Finish();

    m_lumen_visualizer.Render();
}
