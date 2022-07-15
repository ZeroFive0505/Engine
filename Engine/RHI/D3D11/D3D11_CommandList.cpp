#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_CommandList.h"
#include "../RHI_Pipeline.h"
#include "../RHI_Device.h"
#include "../RHI_Sampler.h"
#include "../RHI_Texture.h"
#include "../RHI_Shader.h"
#include "../RHI_ConstantBuffer.h"
#include "../RHI_VertexBuffer.h"
#include "../RHI_IndexBuffer.h"
#include "../RHI_StructuredBuffer.h"
#include "../RHI_BlendState.h"
#include "../RHI_DepthStencilState.h"
#include "../RHI_RasterizerState.h"
#include "../RHI_InputLayout.h"
#include "../RHI_SwapChain.h"
#include "../RHI_PipelineState.h"
#include "../../Profiling/Profiler.h"
#include "../../Rendering/Renderer.h"

using namespace std;

namespace PlayGround
{
    bool RHI_CommandList::m_memory_query_support = true;

    RHI_CommandList::RHI_CommandList(Context* context, void* cmd_pool, const char* name) : EngineObject(context)
    {
        m_renderer = context->GetSubModule<Renderer>();
        m_profiler = context->GetSubModule<Profiler>();
        m_rhi_device = m_renderer->GetRhiDevice().get();
        m_ObjectName = name;
        m_timestamps.fill(0);
    }

    RHI_CommandList::~RHI_CommandList() = default;

    void RHI_CommandList::Begin()
    {
        m_state = RHI_CommandListState::Recording;
    }

    bool RHI_CommandList::End()
    {
        m_state = RHI_CommandListState::Ended;
        return true;
    }

    bool RHI_CommandList::Submit()
    {
        m_state = RHI_CommandListState::Submitted;
        return true;
    }

    bool RHI_CommandList::Reset()
    {
        m_state = RHI_CommandListState::Idle;
        return true;
    }

    bool RHI_CommandList::BeginRenderPass(RHI_PipelineState& pipeline_state)
    {
        ASSERT(pipeline_state.IsValid());

        UnbindOutputTextures();

        m_pipeline_state = pipeline_state;

        Timeblock_Start(pipeline_state.pass_name, pipeline_state.profile, pipeline_state.gpu_marker);

        ID3D11DeviceContext* device_context = m_rhi_device->GetContextRhi()->device_context;

        {
            ID3D11InputLayout* input_layout = static_cast<ID3D11InputLayout*>(pipeline_state.shader_vertex ? pipeline_state.shader_vertex->GetInputLayout()->GetResource() : nullptr);

            ID3D11InputLayout* input_layout_set = nullptr;
            device_context->IAGetInputLayout(&input_layout_set);

            if (input_layout_set != input_layout)
            {
                device_context->IASetInputLayout(input_layout);
            }
        }

        {
            ID3D11VertexShader* shader = static_cast<ID3D11VertexShader*>(pipeline_state.shader_vertex ? pipeline_state.shader_vertex->GetResource() : nullptr);

            ID3D11VertexShader* set_shader = nullptr; UINT instance_count = 256; ID3D11ClassInstance* instances[256];
            device_context->VSGetShader(&set_shader, instances, &instance_count);

            if (set_shader != shader)
            {
                device_context->VSSetShader(shader, nullptr, 0);

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_shader_vertex++;
                }
            }
        }

        {
            ID3D11PixelShader* shader = static_cast<ID3D11PixelShader*>(pipeline_state.shader_pixel ? pipeline_state.shader_pixel->GetResource() : nullptr);

            ID3D11PixelShader* set_shader = nullptr; UINT instance_count = 256; ID3D11ClassInstance* instances[256];
            device_context->PSGetShader(&set_shader, instances, &instance_count);

            if (set_shader != shader)
            {
                device_context->PSSetShader(shader, nullptr, 0);

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_shader_pixel++;
                }
            }
        }

        {
            ID3D11ComputeShader* shader = static_cast<ID3D11ComputeShader*>(pipeline_state.shader_compute ? pipeline_state.shader_compute->GetResource() : nullptr);

            ID3D11ComputeShader* set_shader = nullptr; UINT instance_count = 256; ID3D11ClassInstance* instances[256];
            device_context->CSGetShader(&set_shader, instances, &instance_count);

            if (set_shader != shader)
            {
                device_context->CSSetShader(shader, nullptr, 0);

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_shader_compute++;
                }
            }
        }

        {
            ID3D11BlendState* blend_state_set = nullptr;
            std::array<FLOAT, 4> blend_factor_set = { 0.0f };
            UINT mask_set = 0;
            device_context->OMGetBlendState(&blend_state_set, blend_factor_set.data(), &mask_set);

            ID3D11BlendState* blend_state = static_cast<ID3D11BlendState*>(pipeline_state.blend_state ? pipeline_state.blend_state->GetResource() : nullptr);
            const float blendFactor = pipeline_state.blend_state ? pipeline_state.blend_state->GetBlendFactor() : 0.0f;
            std::array<FLOAT, 4> blend_factor = { blendFactor, blendFactor, blendFactor, blendFactor };
            const UINT mask = 0;

            if (blend_state_set != blend_state || blend_factor_set != blend_factor || mask_set != mask)
            {
                device_context->OMSetBlendState(blend_state, blend_factor.data(), 0xffffffff);
            }
        }

        {
            ID3D11DepthStencilState* depth_stencil_state = static_cast<ID3D11DepthStencilState*>(pipeline_state.depth_stencil_state ? pipeline_state.depth_stencil_state->GetResource() : nullptr);

            ID3D11DepthStencilState* depth_stencil_state_set = nullptr;
            UINT stencil_ref = 0;
            device_context->OMGetDepthStencilState(&depth_stencil_state_set, &stencil_ref);

            if (depth_stencil_state_set != depth_stencil_state)
            {
                device_context->OMSetDepthStencilState(depth_stencil_state, 1);
            }
        }

        {
            ID3D11RasterizerState* rasterizer_state = static_cast<ID3D11RasterizerState*>(pipeline_state.rasterizer_state ? pipeline_state.rasterizer_state->GetResource() : nullptr);

            ID3D11RasterizerState* rasterizer_state_set = nullptr;

            device_context->RSGetState(&rasterizer_state_set);
            if (rasterizer_state_set != rasterizer_state)
            {
                device_context->RSSetState(rasterizer_state);
            }
        }

        if (pipeline_state.primitive_topology != RHI_PrimitiveTopology_Mode::Undefined)
        {
            const D3D11_PRIMITIVE_TOPOLOGY topology = d3d11_primitive_topology[static_cast<uint32_t>(pipeline_state.primitive_topology)];

            D3D11_PRIMITIVE_TOPOLOGY topology_set;
            device_context->IAGetPrimitiveTopology(&topology_set);

            if (topology_set != topology)
            {
                device_context->IASetPrimitiveTopology(d3d11_primitive_topology[static_cast<uint32_t>(pipeline_state.primitive_topology)]);
            }
        }

        {
            ID3D11DepthStencilView* depth_stencil = nullptr;
            if (pipeline_state.render_target_depth_texture)
            {
                ASSERT(pipeline_state.render_target_depth_texture->IsRenderTargetDepthStencil());

                if (pipeline_state.render_target_depth_texture_read_only)
                {
                    depth_stencil = static_cast<ID3D11DepthStencilView*>(pipeline_state.render_target_depth_texture->GetResource_View_DepthStencilReadOnly(pipeline_state.render_target_depth_stencil_texture_array_index));
                }
                else
                {
                    depth_stencil = static_cast<ID3D11DepthStencilView*>(pipeline_state.render_target_depth_texture->GetResource_View_DepthStencil(pipeline_state.render_target_depth_stencil_texture_array_index));
                }
            }

            std::array<ID3D11RenderTargetView*, rhi_max_render_target_count> render_targets = { nullptr };
            {
                if (pipeline_state.render_target_swapchain)
                {
                    render_targets[0] = { static_cast<ID3D11RenderTargetView*>(pipeline_state.render_target_swapchain->Get_Resource_View_RenderTarget()) };
                }
                else
                {
                    for (uint8_t i = 0; i < rhi_max_render_target_count; i++)
                    {
                        if (pipeline_state.render_target_color_textures[i])
                        {
                            ASSERT(pipeline_state.render_target_color_textures[i]->IsRenderTargetColor());

                            ID3D11RenderTargetView* rt = static_cast<ID3D11RenderTargetView*>(pipeline_state.render_target_color_textures[i]->GetResource_View_RenderTarget(pipeline_state.render_target_color_texture_array_index));
                            render_targets[i] = rt;
                        }
                    }
                }
            }

            {
                std::array<ID3D11RenderTargetView*, rhi_max_render_target_count> set_render_target_views = { nullptr };
                ID3D11DepthStencilView* set_depth_stencil_view = nullptr;
                device_context->OMGetRenderTargets(rhi_max_render_target_count, set_render_target_views.data(), &set_depth_stencil_view);

                if (render_targets != set_render_target_views || depth_stencil != set_depth_stencil_view)
                {
                    UINT render_target_count = 0;
                    for (ID3D11RenderTargetView* rt : render_targets)
                    {
                        if (rt)
                        {
                            render_target_count++;
                        }
                    }

                    device_context->OMSetRenderTargets
                    (
                        render_target_count,
                        reinterpret_cast<ID3D11RenderTargetView* const*>(render_targets.data()),
                        depth_stencil
                    );

                    if (m_profiler)
                    {
                        m_profiler->m_Rhi_bindings_render_target++;
                    }
                }
            }
        }

        if (pipeline_state.viewport.IsDefined())
        {
            SetViewport(pipeline_state.viewport);
        }

        ClearPipelineStateRenderTargets(pipeline_state);

        m_renderer->SetGlobalShaderResources(this);

        if (m_profiler)
        {
            m_profiler->m_Rhi_bindings_pipeline++;
        }

        return true;
    }

    void RHI_CommandList::EndRenderPass()
    {
        Timeblock_End();
    }

    void RHI_CommandList::ClearPipelineStateRenderTargets(RHI_PipelineState& pipeline_state)
    {
        for (uint8_t i = 0; i < rhi_max_render_target_count; i++)
        {
            if (pipeline_state.clear_color[i] != rhi_color_load && pipeline_state.clear_color[i] != rhi_color_dont_care)
            {
                if (pipeline_state.render_target_swapchain)
                {
                    m_rhi_device->GetContextRhi()->device_context->ClearRenderTargetView
                    (
                        static_cast<ID3D11RenderTargetView*>(const_cast<void*>(pipeline_state.render_target_swapchain->Get_Resource_View_RenderTarget())),
                        pipeline_state.clear_color[i].Data()
                    );
                }
                else if (pipeline_state.render_target_color_textures[i])
                {
                    m_rhi_device->GetContextRhi()->device_context->ClearRenderTargetView
                    (
                        static_cast<ID3D11RenderTargetView*>(const_cast<void*>(pipeline_state.render_target_color_textures[i]->GetResource_View_RenderTarget(pipeline_state.render_target_color_texture_array_index))),
                        pipeline_state.clear_color[i].Data()
                    );
                }
            }
        }

        if (pipeline_state.render_target_depth_texture)
        {
            UINT clear_flags = 0;
            clear_flags |= (pipeline_state.clear_depth != rhi_depth_stencil_load && pipeline_state.clear_depth != rhi_depth_stencil_dont_care) ? D3D11_CLEAR_DEPTH : 0;
            clear_flags |= (pipeline_state.clear_stencil != rhi_depth_stencil_load && pipeline_state.clear_stencil != rhi_depth_stencil_dont_care) ? D3D11_CLEAR_STENCIL : 0;
            if (clear_flags != 0)
            {
                m_rhi_device->GetContextRhi()->device_context->ClearDepthStencilView
                (
                    static_cast<ID3D11DepthStencilView*>(pipeline_state.render_target_depth_texture->GetResource_View_DepthStencil(pipeline_state.render_target_depth_stencil_texture_array_index)),
                    clear_flags,
                    static_cast<FLOAT>(pipeline_state.clear_depth),
                    static_cast<UINT8>(pipeline_state.clear_stencil)
                );
            }
        }
    }

    void RHI_CommandList::ClearRenderTarget(RHI_Texture* texture,
        const uint32_t color_index          /*= 0*/,
        const uint32_t depth_stencil_index  /*= 0*/,
        const bool storage                  /*= false*/,
        const Math::Vector4& clear_color    /*= rhi_color_load*/,
        const float clear_depth             /*= rhi_depth_load*/,
        const float clear_stencil           /*= rhi_stencil_load*/
    )
    {
        ASSERT(texture->CanBeCleared());

        if (storage)
        {
            ASSERT(texture->IsUav());

            if (clear_color == rhi_color_load || clear_color == rhi_color_dont_care)
                return;

            m_rhi_device->GetContextRhi()->device_context->ClearUnorderedAccessViewFloat(static_cast<ID3D11UnorderedAccessView*>(texture->GetResource_View_Uav()), clear_color.Data());

            if (texture->HasPerMipViews())
            {
                for (uint32_t i = 0; i < texture->GetMipCount(); i++)
                {
                    m_rhi_device->GetContextRhi()->device_context->ClearUnorderedAccessViewFloat(static_cast<ID3D11UnorderedAccessView*>(texture->GetResource_Views_Uav(i)), clear_color.Data());
                }
            }
        }
        else if (texture->IsRenderTargetColor() || texture->IsRenderTargetDepthStencil())
        {
            if (texture->IsRenderTargetColor())
            {
                if (clear_color == rhi_color_load || clear_color == rhi_color_dont_care)
                    return;

                m_rhi_device->GetContextRhi()->device_context->ClearRenderTargetView
                (
                    static_cast<ID3D11RenderTargetView*>(const_cast<void*>(texture->GetResource_View_RenderTarget(color_index))),
                    clear_color.Data()
                );
            }
            else if (texture->IsRenderTargetDepthStencil())
            {
                if ((clear_depth == rhi_depth_stencil_load || clear_depth == rhi_depth_stencil_dont_care) && (clear_stencil == rhi_depth_stencil_load || clear_stencil == rhi_depth_stencil_dont_care))
                    return;

                UINT clear_flags = 0;
                clear_flags |= (clear_depth != rhi_depth_stencil_load && clear_depth != rhi_depth_stencil_dont_care) ? D3D11_CLEAR_DEPTH : 0;
                clear_flags |= (clear_stencil != rhi_depth_stencil_load && clear_stencil != rhi_depth_stencil_dont_care) ? D3D11_CLEAR_STENCIL : 0;
                if (clear_flags != 0)
                {
                    m_rhi_device->GetContextRhi()->device_context->ClearDepthStencilView
                    (
                        static_cast<ID3D11DepthStencilView*>(texture->GetResource_View_DepthStencil(depth_stencil_index)),
                        clear_flags,
                        static_cast<FLOAT>(clear_depth),
                        static_cast<UINT8>(clear_stencil)
                    );
                }
            }
        }
    }

    void RHI_CommandList::Draw(const uint32_t vertex_count, uint32_t vertex_start_index /*= 0*/)
    {
        m_rhi_device->GetContextRhi()->device_context->Draw(static_cast<UINT>(vertex_count), static_cast<UINT>(vertex_start_index));

        if (m_profiler)
        {
            m_profiler->m_Rhi_draw++;
        }
    }

    void RHI_CommandList::DrawIndexed(const uint32_t index_count, const uint32_t index_offset, const uint32_t vertex_offset)
    {
        m_rhi_device->GetContextRhi()->device_context->DrawIndexed
        (
            static_cast<UINT>(index_count),
            static_cast<UINT>(index_offset),
            static_cast<INT>(vertex_offset)
        );

        if (m_profiler)
        {
            m_profiler->m_Rhi_draw++;
        }
    }

    void RHI_CommandList::Dispatch(uint32_t x, uint32_t y, uint32_t z, bool async /*= false*/)
    {
        ID3D11Device5* device = m_rhi_device->GetContextRhi()->device;
        ID3D11DeviceContext4* device_context = m_rhi_device->GetContextRhi()->device_context;

        device_context->Dispatch(x, y, z);

        if (m_profiler)
        {
            m_profiler->m_Rhi_dispatch++;
        }

        const void* resource_array[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        device_context->CSSetUnorderedAccessViews(0, 8, reinterpret_cast<ID3D11UnorderedAccessView* const*>(&resource_array), nullptr);
    }

    void RHI_CommandList::Blit(RHI_Texture* source, RHI_Texture* destination)
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-copyresource
        ASSERT(source != nullptr);
        ASSERT(destination != nullptr);
        ASSERT(source->GetResource() != nullptr);
        ASSERT(destination->GetResource() != nullptr);
        ASSERT(source->GetObjectID() != destination->GetObjectID());
        ASSERT(source->GetFormat() == destination->GetFormat());
        ASSERT(source->GetWidth() == destination->GetWidth());
        ASSERT(source->GetHeight() == destination->GetHeight());
        ASSERT(source->GetArrayLength() == destination->GetArrayLength());
        ASSERT(source->GetMipCount() == destination->GetMipCount());

        m_rhi_device->GetContextRhi()->device_context->CopyResource(static_cast<ID3D11Resource*>(destination->GetResource()), static_cast<ID3D11Resource*>(source->GetResource()));
    }

    void RHI_CommandList::SetViewport(const RHI_Viewport& viewport) const
    {
        ASSERT(m_state == RHI_CommandListState::Recording);

        D3D11_VIEWPORT d3d11_viewport = {};
        d3d11_viewport.TopLeftX = viewport.x;
        d3d11_viewport.TopLeftY = viewport.y;
        d3d11_viewport.Width = viewport.width;
        d3d11_viewport.Height = viewport.height;
        d3d11_viewport.MinDepth = viewport.depth_min;
        d3d11_viewport.MaxDepth = viewport.depth_max;

        m_rhi_device->GetContextRhi()->device_context->RSSetViewports(1, &d3d11_viewport);
    }

    void RHI_CommandList::SetScissorRectangle(const Math::Rectangle& scissor_rectangle) const
    {
        const D3D11_RECT d3d11_rectangle =
        {
            static_cast<LONG>(scissor_rectangle.left),
            static_cast<LONG>(scissor_rectangle.top),
            static_cast<LONG>(scissor_rectangle.right),
            static_cast<LONG>(scissor_rectangle.bottom)
        };

        m_rhi_device->GetContextRhi()->device_context->RSSetScissorRects(1, &d3d11_rectangle);
    }

    void RHI_CommandList::SetBufferVertex(const RHI_VertexBuffer* buffer, const uint64_t offset /*= 0*/)
    {
        ASSERT(buffer != nullptr);
        ASSERT(buffer->GetResource() != nullptr);

        ID3D11Buffer* vertex_buffer = static_cast<ID3D11Buffer*>(buffer->GetResource());
        UINT stride = buffer->GetStride();
        UINT offsets[] = { static_cast<UINT>(offset) };
        ID3D11DeviceContext* device_context = m_rhi_device->GetContextRhi()->device_context;

        ID3D11Buffer* set_buffer = nullptr;
        UINT set_stride = buffer->GetStride();
        UINT set_offset = 0;
        device_context->IAGetVertexBuffers(0, 1, &set_buffer, &set_stride, &set_offset);

        if (set_buffer == vertex_buffer && set_offset == offset)
            return;

        device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, offsets);

        if (m_profiler)
        {
            m_profiler->m_Rhi_bindings_buffer_vertex++;
        }
    }

    void RHI_CommandList::SetBufferIndex(const RHI_IndexBuffer* buffer, const uint64_t offset /*= 0*/)
    {
        ASSERT(buffer != nullptr);
        ASSERT(buffer->GetResource() != nullptr);

        ID3D11Buffer* index_buffer = static_cast<ID3D11Buffer*>(buffer->GetResource());
        const DXGI_FORMAT format = buffer->Is16Bit() ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        ID3D11DeviceContext* device_context = m_rhi_device->GetContextRhi()->device_context;

        ID3D11Buffer* set_buffer = nullptr;
        DXGI_FORMAT set_format = DXGI_FORMAT_UNKNOWN;
        UINT set_offset = 0;
        device_context->IAGetIndexBuffer(&set_buffer, &set_format, &set_offset);

        if (set_buffer == index_buffer && set_offset == offset)
            return;

        device_context->IASetIndexBuffer(index_buffer, format, static_cast<UINT>(offset));

        if (m_profiler)
        {
            m_profiler->m_Rhi_bindings_buffer_index++;
        }
    }

    void RHI_CommandList::SetConstantBuffer(const uint32_t slot, const uint8_t scope, RHI_ConstantBuffer* constant_buffer) const
    {
        void* buffer = static_cast<ID3D11Buffer*>(constant_buffer ? constant_buffer->GetResource() : nullptr);
        const void* buffer_array[1] = { buffer };
        const UINT range = 1;
        ID3D11DeviceContext* device_context = m_rhi_device->GetContextRhi()->device_context;

        if (scope & RHI_Shader_Vertex)
        {
            ID3D11Buffer* set_buffer = nullptr;
            device_context->VSGetConstantBuffers(slot, range, &set_buffer);
            if (set_buffer != buffer)
            {
                device_context->VSSetConstantBuffers(slot, range, reinterpret_cast<ID3D11Buffer* const*>(range > 1 ? buffer : &buffer_array));

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_buffer_constant++;
                }
            }
        }

        if (scope & RHI_Shader_Pixel)
        {
            ID3D11Buffer* set_buffer = nullptr;
            device_context->PSGetConstantBuffers(slot, range, &set_buffer);
            if (set_buffer != buffer)
            {
                device_context->PSSetConstantBuffers(slot, range, reinterpret_cast<ID3D11Buffer* const*>(range > 1 ? buffer : &buffer_array));

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_buffer_constant++;
                }
            }
        }

        if (scope & RHI_Shader_Compute)
        {
            ID3D11Buffer* set_buffer = nullptr;
            device_context->CSGetConstantBuffers(slot, range, &set_buffer);
            if (set_buffer != buffer)
            {
                device_context->CSSetConstantBuffers(slot, range, reinterpret_cast<ID3D11Buffer* const*>(range > 1 ? buffer : &buffer_array));

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_buffer_constant++;
                }
            }
        }
    }

    void RHI_CommandList::SetSampler(const uint32_t slot, RHI_Sampler* sampler) const
    {
        const UINT start_slot = slot;
        const UINT range = 1;
        const void* sampler_array[1] = { sampler ? sampler->GetResource() : nullptr };
        ID3D11DeviceContext* device_context = m_rhi_device->GetContextRhi()->device_context;

        if (m_pipeline_state.IsCompute())
        {
            ID3D11SamplerState* set_sampler = nullptr;
            device_context->CSGetSamplers(start_slot, range, &set_sampler);
            if (set_sampler != sampler_array[0])
            {
                device_context->CSSetSamplers(start_slot, range, reinterpret_cast<ID3D11SamplerState* const*>(&sampler_array));

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_sampler++;
                }
            }
        }
        else
        {
            ID3D11SamplerState* set_sampler = nullptr;
            device_context->PSGetSamplers(start_slot, range, &set_sampler);
            if (set_sampler != sampler_array[0])
            {
                device_context->PSSetSamplers(start_slot, range, reinterpret_cast<ID3D11SamplerState* const*>(&sampler_array));

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_sampler++;
                }
            }
        }
    }

    void RHI_CommandList::SetTexture(const uint32_t slot, RHI_Texture* texture, const int mip /*= -1*/, const bool ranged /*= false*/, const bool uav /*= false*/)
    {
        if (texture)
        {
            if (uav)
            {
                ASSERT(texture->IsUav());
            }
            else
            {
                ASSERT(texture->IsSrv());
            }
        }

        bool mip_requested = mip != -1;
        const UINT range = ranged ? (texture->GetMipCount() - (mip_requested ? mip : 0)) : 1;
        ID3D11DeviceContext* device_context = m_rhi_device->GetContextRhi()->device_context;

        array<void*, m_resource_array_length_max> resources;
        resources.fill(nullptr);
        if (texture)
        {
            if (!ranged)
            {
                if (uav)
                {
                    resources[0] = mip_requested ? texture->GetResource_Views_Uav(mip) : texture->GetResource_View_Uav();
                }
                else
                {
                    resources[0] = mip_requested ? texture->GetResource_Views_Srv(mip) : texture->GetResource_View_Srv();
                }
            }
            else
            {
                for (uint32_t i = 0; i < range; i++)
                {
                    uint32_t mip_offset = mip + i;
                    resources[i] = uav ? texture->GetResource_Views_Uav(mip_offset) : texture->GetResource_Views_Srv(mip_offset);
                }
            }
        }

        if (uav)
        {
            array<void*, m_resource_array_length_max> set_resources;
            set_resources.fill(nullptr);

            device_context->CSGetUnorderedAccessViews(slot, range, reinterpret_cast<ID3D11UnorderedAccessView**>(&set_resources[0]));
            if (set_resources != resources)
            {
                device_context->CSSetUnorderedAccessViews(slot, range, reinterpret_cast<ID3D11UnorderedAccessView* const*>(resources.data()), nullptr);

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_texture_storage++;
                }

                m_output_textures[m_output_textures_index].texture = texture;
                m_output_textures[m_output_textures_index].slot = slot;
                m_output_textures[m_output_textures_index].mip = mip;
                m_output_textures[m_output_textures_index].ranged = ranged;
                m_output_textures_index++;
            }
        }
        else
        {
            const uint8_t scope = m_pipeline_state.IsCompute() ? RHI_Shader_Compute : RHI_Shader_Pixel;

            array<void*, m_resource_array_length_max> set_resources;
            set_resources.fill(nullptr);

            if (scope & RHI_Shader_Pixel)
            {
                device_context->PSGetShaderResources(slot, range, reinterpret_cast<ID3D11ShaderResourceView**>(&set_resources[0]));
                if (set_resources != resources)
                {
                    device_context->PSSetShaderResources(slot, range, reinterpret_cast<ID3D11ShaderResourceView* const*>(resources.data()));

                    if (m_profiler)
                    {
                        m_profiler->m_Rhi_bindings_texture_sampled++;
                    }
                }
            }
            else if (scope & RHI_Shader_Compute)
            {
                device_context->CSGetShaderResources(slot, range, reinterpret_cast<ID3D11ShaderResourceView**>(&set_resources[0]));
                if (set_resources != resources)
                {
                    device_context->CSSetShaderResources(slot, range, reinterpret_cast<ID3D11ShaderResourceView* const*>(resources.data()));

                    if (m_profiler)
                    {
                        m_profiler->m_Rhi_bindings_texture_sampled++;
                    }
                }
            }
        }
    }

    void RHI_CommandList::SetStructuredBuffer(const uint32_t slot, RHI_StructuredBuffer* structured_buffer) const
    {
        array<void*, 1> view_array = { structured_buffer ? structured_buffer->GetResourceUav() : nullptr };
        const UINT range = 1;
        ID3D11DeviceContext* device_context = m_rhi_device->GetContextRhi()->device_context;

        ID3D11UnorderedAccessView* set_uav = nullptr;
        device_context->CSGetUnorderedAccessViews(slot, range, &set_uav);
        if (set_uav != view_array[0])
        {
            device_context->CSSetUnorderedAccessViews(slot, range, reinterpret_cast<ID3D11UnorderedAccessView* const*>(&view_array), nullptr);

            if (m_profiler)
            {
                m_profiler->m_Rhi_bindings_buffer_structured++;
            }
        }
    }

    void RHI_CommandList::Timestamp_Start(void* query)
    {
        ASSERT(m_rhi_device);
        m_rhi_device->QueryEnd(query);
    }

    void RHI_CommandList::Timestamp_End(void* query)
    {
        ASSERT(m_rhi_device);
        m_rhi_device->QueryEnd(query);
    }

    float RHI_CommandList::Timestamp_GetDuration(void* query_start, void* query_end, const uint32_t pass_index)
    {
        ASSERT(query_start != nullptr);
        ASSERT(query_end != nullptr);
        ASSERT(m_rhi_device != nullptr);

        RHI_Context* rhi_context = m_rhi_device->GetContextRhi();

        uint64_t start_time = 0;
        rhi_context->device_context->GetData(static_cast<ID3D11Query*>(query_start), &start_time, sizeof(start_time), 0);

        uint64_t end_time = 0;
        rhi_context->device_context->GetData(static_cast<ID3D11Query*>(query_end), &end_time, sizeof(end_time), 0);

        const uint64_t delta = end_time - start_time;
        const double duration_ms = (delta * 1000.0) / static_cast<double>(m_rhi_device->GetTimestampPeriod());

        return static_cast<float>(duration_ms);
    }

    uint32_t RHI_CommandList::Gpu_GetMemoryUsed(RHI_Device* rhi_device)
    {
        if (!m_memory_query_support)
            return 0;

        if (const PhysicalDevice* physical_device = rhi_device->GetPrimaryPhysicalDevice())
        {
            if (IDXGIAdapter3* adapter = static_cast<IDXGIAdapter3*>(physical_device->GetData()))
            {
                DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
                const HRESULT result = adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);

                if (SUCCEEDED(result))
                {
                    return static_cast<uint32_t>(info.CurrentUsage / 1024 / 1024);
                }
                else
                {
                    LOG_ERROR("Failed to get adapter memory info, %s", d3d11_utility::dxgi_error_to_string(result));
                    m_memory_query_support = false;
                }
            }
        }

        return 0;
    }

    void RHI_CommandList::Timeblock_Start(const char* name, const bool profile, const bool gpu_markers)
    {
        ASSERT(name != nullptr);

        RHI_Context* rhi_context = m_rhi_device->GetContextRhi();

        if (rhi_context->profiler && profile)
        {
            if (m_profiler)
            {
                m_profiler->TimeBlockStart(name, TimeBlockType::CPU, this);
                m_profiler->TimeBlockStart(name, TimeBlockType::GPU, this);
            }
        }

        if (rhi_context->gpu_markers && gpu_markers)
        {
            m_rhi_device->GetContextRhi()->annotation->BeginEvent(FileSystem::StringToWstring(name).c_str());
        }
    }

    void RHI_CommandList::Timeblock_End()
    {
        RHI_Context* rhi_context = m_rhi_device->GetContextRhi();
        if (rhi_context->gpu_markers && m_pipeline_state.gpu_marker)
        {
            rhi_context->annotation->EndEvent();
        }

        if (rhi_context->profiler && m_pipeline_state.profile)
        {
            if (m_profiler)
            {
                m_profiler->TimeBlockEnd();
                m_profiler->TimeBlockEnd();
            }
        }
    }

    void RHI_CommandList::StartMarker(const char* name)
    {
        if (m_rhi_device->GetContextRhi()->gpu_markers)
        {
            m_rhi_device->GetContextRhi()->annotation->BeginEvent(FileSystem::StringToWstring(name).c_str());
        }
    }

    void RHI_CommandList::EndMarker()
    {
        if (m_rhi_device->GetContextRhi()->gpu_markers)
        {
            m_rhi_device->GetContextRhi()->annotation->EndEvent();
        }
    }

    void RHI_CommandList::OnDraw()
    {

    }

    void RHI_CommandList::UnbindOutputTextures()
    {
        ID3D11DeviceContext* device_context = m_rhi_device->GetContextRhi()->device_context;

        array<void*, m_resource_array_length_max> resources;
        resources.fill(nullptr);

        for (OutputTexture& texture : m_output_textures)
        {
            if (texture.texture)
            {
                bool mip_requested = texture.mip != -1;
                const UINT range = texture.ranged ? (texture.texture->GetMipCount() - (mip_requested ? texture.mip : 0)) : 1;

                device_context->CSSetUnorderedAccessViews(texture.slot, range, reinterpret_cast<ID3D11UnorderedAccessView* const*>(resources.data()), nullptr);

                if (m_profiler)
                {
                    m_profiler->m_Rhi_bindings_texture_storage++;
                }

                texture.texture = nullptr;
            }
        }

        m_output_textures_index = 0;
    }

    void RHI_CommandList::Descriptors_GetLayoutFromPipelineState(RHI_PipelineState& pipeline_state)
    {

    }
}