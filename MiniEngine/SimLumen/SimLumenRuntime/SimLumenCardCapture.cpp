#include "SimLumenCardCapture.h"

using namespace Graphics;

void CSimLuCardCapturer::Init(SCardCaptureInitDesc& init_desc)
{
    CreatePSO(init_desc);

    m_need_updata_cards = true;
    m_gen_cards = true;
}

void CSimLuCardCapturer::UpdateSceneCards(std::vector<SLumenMeshInstance>& mesh_instances, DescriptorHeap* desc_heap)
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Card Capture");

    if (m_gen_cards)
    {
        temp_cards.resize(mesh_instances.size() * 6);
        for (int idx = 0; idx < temp_cards.size(); idx += 6)
        {
            for (int dir_idx = 0; dir_idx < 6; dir_idx++)//justy for simplify
            {
                temp_cards[idx + dir_idx].m_temp_card_albedo.Create(L"m_temp_card_albedo", 128, 128, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
                temp_cards[idx + dir_idx].m_temp_card_normal.Create(L"m_temp_card_normal", 128, 128, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
                temp_cards[idx + dir_idx].m_temp_card_depth.Create(L"m_temp_card_depth", 128, 128, 1, DXGI_FORMAT_D32_FLOAT);
            }
        }
        m_gen_cards = false;
    }


    {
        for (int idx = 0; idx < temp_cards.size(); idx++)
        {
            gfxContext.TransitionResource(temp_cards[idx].m_temp_card_albedo, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
            gfxContext.TransitionResource(temp_cards[idx].m_temp_card_normal, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
            gfxContext.TransitionResource(temp_cards[idx].m_temp_card_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, false);

            gfxContext.ClearColor(temp_cards[idx].m_temp_card_albedo);
            gfxContext.ClearColor(temp_cards[idx].m_temp_card_normal);
            gfxContext.ClearDepth(temp_cards[idx].m_temp_card_depth);
        }

        gfxContext.FlushResourceBarriers();
        gfxContext.SetViewportAndScissor(0, 0, 128, 128);
        gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, desc_heap->GetHeapPointer());
        gfxContext.SetRootSignature(m_card_capture_root_sig);
        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.SetPipelineState(m_card_capture_pso);

        Vector3 light_direction_neg[6] = { Vector3(0,0,-1),Vector3(0,0,1), Vector3(0,-1,0), Vector3(0,1,0), Vector3(-1,0,0), Vector3(1,0,0) };

        for (int idx = 0; idx < mesh_instances.size(); idx++)
        {
            SLumenMeshInstance& lumen_mesh_instance = mesh_instances[idx];
            CSimLumenMeshResouce& mesh_resouce = mesh_instances[idx].m_mesh_resource;

            {
                DynAlloc mesh_consatnt = gfxContext.ReserveUploadMemory(sizeof(SLumenMeshConstant));
                memcpy(mesh_consatnt.DataPtr, &lumen_mesh_instance.m_LumenConstant, sizeof(SLumenMeshConstant));
                gfxContext.SetConstantBuffer(0, mesh_consatnt.GpuAddress);
            }

            for (int dir = 0; dir < 6; dir++)
            {
                {
                    // 0 / 1 -> 0 ->  2
                    // 2 / 3 -> 1 ->  1
                    // 4 / 5 -> 2 ->  0

                    int dimension = 2 - dir / 2;
                    int up_dir = (dimension + 3 - 1) % 3;

                    XMFLOAT3 up_direction(0, 0, 0);
                    AddFloatComponent(up_direction, up_dir, 1.0);

                    SLumenMeshCards& mesh_card = mesh_resouce.m_cards[dir];
                    Math::BoundingBox& bound_box = mesh_card.m_local_boundbox;
                    Vector3 bound_extent = bound_box.Extents;
                    Vector3 bound_center = bound_box.Center;

                    XMMATRIX proj_matrix = XMMatrixOrthographicRH(
                        GetFloatComponent(bound_box.Extents, (dimension + 1) % 3) * 2,
                        GetFloatComponent(bound_box.Extents, (dimension + 2) % 3) * 2,
                        0,
                        GetFloatComponent(bound_box.Extents, dimension) * 2);

                    m_card_camera.SetEyeAtUp(bound_center - (bound_extent + Vector3(1e-5, 1e-5, 1e-5)) * light_direction_neg[dir], bound_center, up_direction);
                    m_card_camera.SetProjMatrix(Matrix4(proj_matrix));
                    m_card_camera.Update();
                }

                {
                    DynAlloc view_consatnt = gfxContext.ReserveUploadMemory(sizeof(SCardCaptureConstant));
                    memcpy(view_consatnt.DataPtr, &m_card_camera.GetViewProjMatrix(), sizeof(SCardCaptureConstant));
                    gfxContext.SetConstantBuffer(1, view_consatnt.GpuAddress);
                }

                D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2] = { temp_cards[idx * 6 + dir].m_temp_card_albedo.GetRTV(),temp_cards[idx * 6 + dir].m_temp_card_normal.GetRTV() };
                gfxContext.SetRenderTargets(2, rtvs, temp_cards[idx * 6 + dir].m_temp_card_depth.GetDSV());
                gfxContext.SetDescriptorTable(2, (*desc_heap)[lumen_mesh_instance.m_tex_table_idx]);
                gfxContext.SetVertexBuffer(0, lumen_mesh_instance.m_vertex_pos_buffer.VertexBufferView());
                gfxContext.SetVertexBuffer(1, lumen_mesh_instance.m_vertex_norm_buffer.VertexBufferView());
                gfxContext.SetVertexBuffer(2, lumen_mesh_instance.m_vertex_uv_buffer.VertexBufferView());
                gfxContext.SetIndexBuffer(lumen_mesh_instance.m_index_buffer.IndexBufferView());

                gfxContext.DrawIndexed(lumen_mesh_instance.m_mesh_resource.m_indices.size());
            }
        }
    }
 
    //
    {
        gfxContext.TransitionResource(g_atlas_albedo, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
        gfxContext.TransitionResource(g_atlas_normal, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
        gfxContext.TransitionResource(g_atlas_depth, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[3] = { g_atlas_albedo.GetRTV(),g_atlas_normal.GetRTV(),g_atlas_depth.GetRTV() };
        gfxContext.SetViewportAndScissor(0, 0, GetLumenConfig().m_atlas_size.x, GetLumenConfig().m_atlas_size.y);
        gfxContext.SetRootSignature(m_card_copy_root_sig);
        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.SetPipelineState(m_card_copy_pso);
        gfxContext.SetVertexBuffer(0, GetGlobalRender()->GetFullScreenPosBuffer()->VertexBufferView());
        gfxContext.SetVertexBuffer(1, GetGlobalRender()->GetFullScreenUVBuffer()->VertexBufferView());
        gfxContext.SetRenderTargets(3, rtvs);

        for (int idx = 0; idx < temp_cards.size(); idx ++)
        {
            STempCardBuffer& temp_card = temp_cards[idx];
            gfxContext.TransitionResource(temp_card.m_temp_card_albedo, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
            gfxContext.TransitionResource(temp_card.m_temp_card_normal, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
            gfxContext.TransitionResource(temp_card.m_temp_card_depth, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);

            gfxContext.SetDynamicDescriptor(1, 0, temp_card.m_temp_card_albedo.GetSRV());
            gfxContext.SetDynamicDescriptor(1, 1, temp_card.m_temp_card_normal.GetSRV());
            gfxContext.SetDynamicDescriptor(1, 2, temp_card.m_temp_card_depth.GetDepthSRV());

            int dest_x = idx % GetLumenConfig().m_atlas_num_xy.x;
            int dest_y = idx / GetLumenConfig().m_atlas_num_xy.y;
            DynAlloc card_copy_constant = gfxContext.ReserveUploadMemory(sizeof(SCardCopyConstant));
            Math::Vector4 dest_atlas_index_and_scale(dest_x, dest_y, 128.0 / 4096.0, 128.0 / 4096.0);
            memcpy(card_copy_constant.DataPtr, &dest_atlas_index_and_scale, sizeof(SCardCopyConstant));
            gfxContext.SetConstantBuffer(0, card_copy_constant.GpuAddress);

            gfxContext.Draw(6);
        }
    }

    gfxContext.Finish();
}

void CSimLuCardCapturer::CreatePSO(SCardCaptureInitDesc& init_desc)
{
	{
        m_card_capture_root_sig.Reset(3, 1);
        m_card_capture_root_sig.InitStaticSampler(0, SamplerLinearClampDesc);
        m_card_capture_root_sig[0].InitAsConstantBuffer(0);
        m_card_capture_root_sig[1].InitAsConstantBuffer(1);
        m_card_capture_root_sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
        m_card_capture_root_sig.Finalize(L"card_capture_root_sig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        D3D12_INPUT_ELEMENT_DESC pos_norm_uv[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,      1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        DXGI_FORMAT color_rt_foramts[2];

        color_rt_foramts[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        color_rt_foramts[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT depth_foramt = DXGI_FORMAT_D32_FLOAT;

        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = init_desc.m_shader_compiler->Compile(L"Shaders/CardCapture.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = init_desc.m_shader_compiler->Compile(L"Shaders/CardCapture.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        m_card_capture_pso = GraphicsPSO(L"m_card_capture_pso");
        m_card_capture_pso.SetRootSignature(m_card_capture_root_sig);
        m_card_capture_pso.SetRasterizerState(RasterizerDefault);
        m_card_capture_pso.SetBlendState(BlendDisable);
        m_card_capture_pso.SetDepthStencilState(DepthStateReadWrite);
        m_card_capture_pso.SetInputLayout(_countof(pos_norm_uv), pos_norm_uv);
        m_card_capture_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_card_capture_pso.SetRenderTargetFormats(2, color_rt_foramts, depth_foramt);
        m_card_capture_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_card_capture_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_card_capture_pso.Finalize();
	}

    {
        m_card_copy_root_sig.Reset(2, 1);
        m_card_copy_root_sig.InitStaticSampler(0, SamplerPointClampDesc);
        m_card_copy_root_sig[0].InitAsConstantBuffer(0);
        m_card_copy_root_sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, D3D12_SHADER_VISIBILITY_PIXEL);
        m_card_copy_root_sig.Finalize(L"m_card_copy_root_sig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        D3D12_INPUT_ELEMENT_DESC pos_norm_uv[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        DXGI_FORMAT color_rt_foramts[3];

        color_rt_foramts[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        color_rt_foramts[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
        color_rt_foramts[2] = DXGI_FORMAT_R32_FLOAT;

        std::shared_ptr<SCompiledShaderCode> p_vs_shader_code = init_desc.m_shader_compiler->Compile(L"Shaders/CardCopy.hlsl", L"vs_main", L"vs_5_1", nullptr, 0);
        std::shared_ptr<SCompiledShaderCode> p_ps_shader_code = init_desc.m_shader_compiler->Compile(L"Shaders/CardCopy.hlsl", L"ps_main", L"ps_5_1", nullptr, 0);

        m_card_copy_pso = GraphicsPSO(L"m_card_copy_pso");
        m_card_copy_pso.SetRootSignature(m_card_copy_root_sig);
        m_card_copy_pso.SetRasterizerState(RasterizerDefault);
        m_card_copy_pso.SetBlendState(BlendDisable);
        m_card_copy_pso.SetDepthStencilState(DepthStateDisabled);
        m_card_copy_pso.SetInputLayout(_countof(pos_norm_uv), pos_norm_uv);
        m_card_copy_pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_card_copy_pso.SetRenderTargetFormats(3, color_rt_foramts, DXGI_FORMAT_UNKNOWN);
        m_card_copy_pso.SetVertexShader(p_vs_shader_code->GetBufferPointer(), p_vs_shader_code->GetBufferSize());
        m_card_copy_pso.SetPixelShader(p_ps_shader_code->GetBufferPointer(), p_ps_shader_code->GetBufferSize());
        m_card_copy_pso.Finalize();
    }
}
