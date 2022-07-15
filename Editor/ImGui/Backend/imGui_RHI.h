#pragma once

#include <vector>
#include "imGui_RHI.h"
#include "../Source/imgui.h"
#include "Rendering/Renderer.h"
#include "RHI/RHI_Device.h"
#include "RHI/RHI_Shader.h"
#include "RHI/RHI_Texture2D.h"
#include "RHI/RHI_SwapChain.h"
#include "RHI/RHI_BlendState.h"
#include "RHI/RHI_CommandList.h"
#include "RHI/RHI_IndexBuffer.h"
#include "RHI/RHI_VertexBuffer.h"
#include "RHI/RHI_PipelineState.h"
#include "RHI/RHI_RasterizerState.h"
#include "RHI/RHI_DepthStencilState.h"
#include "RHI/RHI_Semaphore.h"
#include "RHI/RHI_CommandPool.h"

#include "Core/Context.h"
#include "Resource/ResourceCache.h"

namespace ImGui::RHI
{
	using namespace PlayGround;
	using namespace Math;
	using namespace std;
    void InitializePlatformInterface();

    Context* g_context = nullptr;
    Renderer* g_renderer = nullptr;

    static shared_ptr<RHI_Device>            g_rhi_device;
    static unique_ptr<RHI_Texture>           g_texture;
    static shared_ptr<RHI_DepthStencilState> g_depth_stencil_state;
    static shared_ptr<RHI_RasterizerState>   g_rasterizer_state;
    static shared_ptr<RHI_BlendState>        g_blend_state;
    static unique_ptr<RHI_Shader>            g_shader_vertex;
    static unique_ptr<RHI_Shader>            g_shader_pixel;
    static RHI_CommandPool* g_cmd_pool;

    static unordered_map<uint64_t, vector<unique_ptr<RHI_VertexBuffer>>> g_vertex_buffers;
    static unordered_map<uint64_t, vector<unique_ptr<RHI_IndexBuffer>>>  g_index_buffers;

    struct WindowData
    {
        uint32_t buffer_count = 2;
        RHI_SwapChain* swapchain;
        RHI_CommandPool* cmd_pool;
        bool image_acquired = false;
    };

    inline bool Initialize(Context* context)
    {
        g_context = context;
        g_renderer = context->GetSubModule<Renderer>();
        g_rhi_device = g_renderer->GetRhiDevice();

        g_cmd_pool = g_rhi_device->AllocateCommandPool("imgui", g_renderer->GetSwapChain()->GetObjectID());
        g_cmd_pool->AllocateCommandLists(g_renderer->GetSwapChain()->GetBufferCount());

        ASSERT(g_context != nullptr);
        ASSERT(g_rhi_device != nullptr);

        {
            g_depth_stencil_state = make_shared<RHI_DepthStencilState>(g_rhi_device, false, false, RHI_Comparison_Function::Always);

            g_rasterizer_state = make_shared<RHI_RasterizerState>
                (
                    g_rhi_device,
                    RHI_CullMode::None,
                    RHI_PolygonMode::Solid,
                    true,  // depth clip
                    true,  // scissor
                    false, // multi-sample
                    false  // anti-aliased lines
                    );

            g_blend_state = make_shared<RHI_BlendState>
                (
                    g_rhi_device,
                    true,
                    RHI_Blend::Src_Alpha,     // source blend
                    RHI_Blend::Inv_Src_Alpha, // destination blend
                    RHI_Blend_Operation::Add, // blend op
                    RHI_Blend::Inv_Src_Alpha, // source blend alpha
                    RHI_Blend::Zero,          // destination blend alpha
                    RHI_Blend_Operation::Add  // destination op alpha
                    );

            {
                const std::string shader_path = g_context->GetSubModule<ResourceCache>()->GetResourceDirectory(EResourceDirectory::Shaders) + "\\ImGui.hlsl";

                bool async = false;

                g_shader_vertex = make_unique<RHI_Shader>(g_context, RHI_Vertex_Type::Pos2DTexCol8);
                g_shader_vertex->Compile(RHI_Shader_Vertex, shader_path, async);

                g_shader_pixel = make_unique<RHI_Shader>(g_context);
                g_shader_pixel->Compile(RHI_Shader_Pixel, shader_path, async);
            }
        }

        {
            unsigned char* pixels;
            int atlas_width, atlas_height, bpp;
            auto& io = GetIO();
            io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height, &bpp);

            const uint32_t size = atlas_width * atlas_height * bpp;
            vector<RHI_Texture_Slice> texture_data;
            vector<std::byte>& mip = texture_data.emplace_back().mips.emplace_back().bytes;
            mip.resize(size);
            mip.reserve(size);
            memcpy(&mip[0], reinterpret_cast<std::byte*>(pixels), size);

            g_texture = make_unique<RHI_Texture2D>(g_context, atlas_width, atlas_height, RHI_Format_R8G8B8A8_Unorm, RHI_Texture_Srv, texture_data, "imgui_font_atlas");
            io.Fonts->TexID = static_cast<ImTextureID>(g_texture.get());
        }

        auto& io = GetIO();
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendRendererName = "RHI";
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            InitializePlatformInterface();
        }

        return true;
    }

    inline void Shutdown()
    {
        DestroyPlatformWindows();
    }

    inline void Render(ImDrawData* draw_data, WindowData* window_data = nullptr, const bool clear = true)
    {
        ASSERT(draw_data != nullptr);

        const int fb_width = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
        const int fb_height = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount == 0)
            return;

        bool is_child_window = window_data != nullptr;
        RHI_SwapChain* swap_chain = is_child_window ? window_data->swapchain : g_renderer->GetSwapChain();
        RHI_CommandPool* cmd_list_pool = is_child_window ? window_data->cmd_pool : g_cmd_pool;

        cmd_list_pool->Update();
        RHI_CommandList* cmd_list = cmd_list_pool->GetCurrentCommandList();
        cmd_list->Begin();

        RHI_VertexBuffer* vertex_buffer = nullptr;
        RHI_IndexBuffer* index_buffer = nullptr;
        {
            const uint64_t swapchain_id = swap_chain->GetObjectID();
            const uint32_t swapchain_cmd_index = g_renderer->GetCmdIndex();

            const uint32_t gap = Math::Util::Clamp<uint32_t>(0, 10, (swapchain_cmd_index + 1) - static_cast<uint32_t>(g_vertex_buffers[swapchain_id].size()));
            for (uint32_t i = 0; i < gap; i++)
            {
                bool is_mappable = true;
                g_vertex_buffers[swapchain_id].emplace_back(make_unique<RHI_VertexBuffer>(g_rhi_device, is_mappable, "imgui"));
                g_index_buffers[swapchain_id].emplace_back(make_unique<RHI_IndexBuffer>(g_rhi_device, is_mappable, "imgui"));
            }

            vertex_buffer = g_vertex_buffers[swapchain_id][swapchain_cmd_index].get();
            index_buffer = g_index_buffers[swapchain_id][swapchain_cmd_index].get();

            if (vertex_buffer->GetVertexCount() < static_cast<unsigned int>(draw_data->TotalVtxCount))
            {
                const unsigned int new_size = draw_data->TotalVtxCount + 5000;
                if (!vertex_buffer->CreateDynamic<ImDrawVert>(new_size))
                    return;
            }

            if (index_buffer->GetIndexCount() < static_cast<unsigned int>(draw_data->TotalIdxCount))
            {
                const unsigned int new_size = draw_data->TotalIdxCount + 10000;
                if (!index_buffer->CreateDynamic<ImDrawIdx>(new_size))
                    return;
            }

            ImDrawVert* vtx_dst = static_cast<ImDrawVert*>(vertex_buffer->Map());
            ImDrawIdx* idx_dst = static_cast<ImDrawIdx*>(index_buffer->Map());
            if (vtx_dst && idx_dst)
            {
                for (auto i = 0; i < draw_data->CmdListsCount; i++)
                {
                    const ImDrawList* imgui_cmd_list = draw_data->CmdLists[i];
                    memcpy(vtx_dst, imgui_cmd_list->VtxBuffer.Data, imgui_cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
                    memcpy(idx_dst, imgui_cmd_list->IdxBuffer.Data, imgui_cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                    vtx_dst += imgui_cmd_list->VtxBuffer.Size;
                    idx_dst += imgui_cmd_list->IdxBuffer.Size;
                }

                vertex_buffer->Unmap();
                index_buffer->Unmap();
            }
        }

        static RHI_PipelineState pipeline_state = {};
        pipeline_state.shader_vertex = g_shader_vertex.get();
        pipeline_state.shader_pixel = g_shader_pixel.get();
        pipeline_state.rasterizer_state = g_rasterizer_state.get();
        pipeline_state.blend_state = g_blend_state.get();
        pipeline_state.depth_stencil_state = g_depth_stencil_state.get();
        pipeline_state.render_target_swapchain = swap_chain;
        pipeline_state.clear_color[0] = clear ? Vector4(0.0f, 0.0f, 0.0f, 1.0f) : rhi_color_dont_care;
        pipeline_state.viewport.width = draw_data->DisplaySize.x;
        pipeline_state.viewport.height = draw_data->DisplaySize.y;
        pipeline_state.dynamic_scissor = true;
        pipeline_state.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
        pipeline_state.pass_name = is_child_window ? "pass_imgui_window_child" : "pass_imgui_window_main";

        pipeline_state.profile = !is_child_window;

        if (cmd_list->BeginRenderPass(pipeline_state))
        {
            Cb_Uber& constant_buffer = g_renderer->GetUberBufferCpu();
            {
                const float L = draw_data->DisplayPos.x;
                const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
                const float T = draw_data->DisplayPos.y;
                const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
                constant_buffer.transform = Matrix
                (
                    2.0f / (R - L), 0.0f, 0.0f, (R + L) / (L - R),
                    0.0f, 2.0f / (T - B), 0.0f, (T + B) / (B - T),
                    0.0f, 0.0f, 0.5f, 0.5f,
                    0.0f, 0.0f, 0.0f, 1.0f
                );
            }

            cmd_list->SetBufferVertex(vertex_buffer);
            cmd_list->SetBufferIndex(index_buffer);

            int global_vtx_offset = 0;
            int global_idx_offset = 0;
            const ImVec2& clip_off = draw_data->DisplayPos;
            Math::Rectangle scissor_rect;
            for (int i = 0; i < draw_data->CmdListsCount; i++)
            {
                ImDrawList* cmd_list_imgui = draw_data->CmdLists[i];
                for (int cmd_i = 0; cmd_i < cmd_list_imgui->CmdBuffer.Size; cmd_i++)
                {
                    const ImDrawCmd* pcmd = &cmd_list_imgui->CmdBuffer[cmd_i];
                    if (pcmd->UserCallback != nullptr)
                    {
                        pcmd->UserCallback(cmd_list_imgui, pcmd);
                    }
                    else
                    {
                        scissor_rect.left = pcmd->ClipRect.x - clip_off.x;
                        scissor_rect.top = pcmd->ClipRect.y - clip_off.y;
                        scissor_rect.right = pcmd->ClipRect.z - clip_off.x;
                        scissor_rect.bottom = pcmd->ClipRect.w - clip_off.y;

                        cmd_list->SetScissorRectangle(scissor_rect);

                        if (RHI_Texture* texture = static_cast<RHI_Texture*>(pcmd->TextureId))
                        {
                            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, texture);

                            if (texture->GetChannelCount() == 1)
                            {
                                texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise);
                                texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_Channel_R);
                            }

                            constant_buffer.options_texture_visualisation = texture->GetFlags();
                        }

                        g_renderer->Update_Cb_Uber(cmd_list);

                        cmd_list->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
                    }

                }
                global_idx_offset += cmd_list_imgui->IdxBuffer.Size;
                global_vtx_offset += cmd_list_imgui->VtxBuffer.Size;
            }

            cmd_list->EndRenderPass();
        }

        if (!is_child_window)
        {
            swap_chain->SetLayout(RHI_Image_Layout::Present_Src, cmd_list);
        }

        cmd_list->End();
        cmd_list->Submit();
    }


    inline WindowData* RHI_GetWindowData(ImGuiViewport* viewport)
    {
        ASSERT(viewport != nullptr);
        return static_cast<WindowData*>(viewport->RendererUserData);
    }

    static void RHI_Window_Create(ImGuiViewport* viewport)
    {
        ASSERT(viewport != nullptr);

        WindowData* window = new WindowData();

        window->swapchain = new RHI_SwapChain
        (
            viewport->PlatformHandleRaw,
            g_rhi_device,
            static_cast<uint32_t>(viewport->Size.x),
            static_cast<uint32_t>(viewport->Size.y),
            RHI_Format_R8G8B8A8_Unorm,
            window->buffer_count,
            RHI_Present_Immediate | RHI_Swap_Flip_Discard,
            (string("swapchain_child_") + string(to_string(viewport->ID))).c_str()
        );

        window->cmd_pool = g_rhi_device->AllocateCommandPool("imgui_child_window", window->swapchain->GetObjectID());
        window->cmd_pool->AllocateCommandLists(window->buffer_count);

        viewport->RendererUserData = window;
    }

    static void RHI_Window_Destroy(ImGuiViewport* viewport)
    {
        ASSERT(viewport != nullptr);

        if (WindowData* window = RHI_GetWindowData(viewport))
        {
            delete window->swapchain;
            delete window->cmd_pool;
            delete window;
        }

        viewport->RendererUserData = nullptr;
    }

    static void RHI_Window_SetSize(ImGuiViewport* viewport, const ImVec2 size)
    {
        ASSERT(viewport != nullptr);

        WindowData* window = RHI_GetWindowData(viewport);
        ASSERT(window != nullptr);

        if (!window->swapchain->Resize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)))
        {
            LOG_ERROR("Failed to resize swap chain");
        }
    }

    static void RHI_Window_Render(ImGuiViewport* viewport, void*)
    {
        ASSERT(viewport != nullptr);

        WindowData* window = RHI_GetWindowData(viewport);
        ASSERT(window != nullptr);

        const bool clear = !(viewport->Flags & ImGuiViewportFlags_NoRendererClear);
        Render(viewport->DrawData, window, clear);

        if (window->swapchain->GetBufferCount() == 1)
        {
            window->image_acquired = true;
        }
    }

    static void RHI_Window_Present(ImGuiViewport* viewport, void*)
    {
        WindowData* window = RHI_GetWindowData(viewport);
        ASSERT(window != nullptr);

        ASSERT(window->cmd_pool->GetCurrentCommandList()->GetState() == PlayGround::RHI_CommandListState::Submitted);

        window->swapchain->Present();
    }

    inline void InitializePlatformInterface()
    {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        platform_io.Renderer_CreateWindow = RHI_Window_Create;
        platform_io.Renderer_DestroyWindow = RHI_Window_Destroy;
        platform_io.Renderer_SetWindowSize = RHI_Window_SetSize;
        platform_io.Renderer_RenderWindow = RHI_Window_Render;
        platform_io.Renderer_SwapBuffers = RHI_Window_Present;
    }
}