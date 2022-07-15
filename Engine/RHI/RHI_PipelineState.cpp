#include "Common.h"
#include "RHI_PipelineState.h"
#include "RHI_Shader.h"
#include "RHI_Texture.h"
#include "RHI_SwapChain.h"
#include "RHI_BlendState.h"
#include "RHI_InputLayout.h"
#include "RHI_RasterizerState.h"
#include "RHI_DepthStencilState.h"
#include "../Utils/Hash.h"

using namespace std;

namespace PlayGround
{
    RHI_PipelineState::RHI_PipelineState()
    {
        clear_color.fill(rhi_color_load);
    }

    RHI_PipelineState::~RHI_PipelineState()
    {

    }

    bool RHI_PipelineState::IsValid()
    {
        // ���������� ����
        bool has_shader_compute = shader_compute ? shader_compute->IsCompiled() : false;
        bool has_shader_vertex = shader_vertex ? shader_vertex->IsCompiled() : false;
        bool has_shader_pixel = shader_pixel ? shader_pixel->IsCompiled() : false;
        bool has_render_target = render_target_color_textures[0] || render_target_depth_texture; // ���� Ÿ���� ������ �Ǿ� �ִ���
        bool has_backbuffer = render_target_swapchain;                                        // ����� ����
        bool has_graphics_states = rasterizer_state && blend_state && depth_stencil_state && primitive_topology != RHI_PrimitiveTopology_Mode::Undefined;
        bool is_graphics_pso = (has_shader_vertex || has_shader_pixel) && !has_shader_compute;
        bool is_compute_pso = has_shader_compute && (!has_shader_vertex && !has_shader_pixel);

        // ���������ο� ���ε��� ���̴��� ���� ���� �ִ�(��� ���� �����͸� �������ҽÿ�)

        // ���� �׷��� ���������������� ����ε� ������Ʈ�� �������� �ʾ����� ��ȯ
        if (is_graphics_pso && !has_graphics_states)
        {
            return false;
        }

        // ���� Ÿ���� �����Ǿ� �ִ��� �ʴٸ�
        if (is_graphics_pso && !has_render_target && !has_backbuffer)
        {
            // �Ѵ� ���ε� �Ǿ����� �ʾƵ� ��ȯ
            if (!has_render_target && !has_backbuffer)
            {
                return false;
            }

            // �Ѵ� ���ε� �Ǿ��־ ��ȯ
            if (has_render_target && has_backbuffer)
            {
                return false;
            }
        }

        return true;
    }

    uint32_t RHI_PipelineState::GetWidth() const
    {
        if (render_target_swapchain)
            return render_target_swapchain->GetWidth();

        if (render_target_color_textures[0])
            return render_target_color_textures[0]->GetWidth();

        if (render_target_depth_texture)
            return render_target_depth_texture->GetWidth();

        return 0;
    }

    uint32_t RHI_PipelineState::GetHeight() const
    {
        if (render_target_swapchain)
            return render_target_swapchain->GetHeight();

        if (render_target_color_textures[0])
            return render_target_color_textures[0]->GetHeight();

        if (render_target_depth_texture)
            return render_target_depth_texture->GetHeight();

        return 0;
    }

    bool RHI_PipelineState::HasClearValues()
    {
        if (clear_depth != rhi_depth_stencil_load && clear_depth != rhi_depth_stencil_dont_care)
            return true;

        if (clear_stencil != rhi_depth_stencil_load && clear_stencil != rhi_depth_stencil_dont_care)
            return true;

        for (const Math::Vector4& color : clear_color)
        {
            if (color != rhi_color_load && color != rhi_color_dont_care)
                return true;
        }

        return false;
    }

    uint32_t RHI_PipelineState::ComputeHash() const
    {
        // ���������� ���� �ؽ� ��ȣ�� �ο��Ѵ�.

        uint32_t hash = 0;

        Utility::Hash::HashCombine(hash, can_use_vertex_index_buffers);
        Utility::Hash::HashCombine(hash, dynamic_scissor);
        Utility::Hash::HashCombine(hash, viewport.x);
        Utility::Hash::HashCombine(hash, viewport.y);
        Utility::Hash::HashCombine(hash, viewport.width);
        Utility::Hash::HashCombine(hash, viewport.height);
        Utility::Hash::HashCombine(hash, primitive_topology);
        Utility::Hash::HashCombine(hash, render_target_color_texture_array_index);
        Utility::Hash::HashCombine(hash, render_target_depth_stencil_texture_array_index);
        Utility::Hash::HashCombine(hash, render_target_swapchain ? render_target_swapchain->GetObjectID() : 0);

        if (!dynamic_scissor)
        {
            Utility::Hash::HashCombine(hash, scissor.left);
            Utility::Hash::HashCombine(hash, scissor.top);
            Utility::Hash::HashCombine(hash, scissor.right);
            Utility::Hash::HashCombine(hash, scissor.bottom);
        }

        if (rasterizer_state)
        {
            Utility::Hash::HashCombine(hash, rasterizer_state->GetObjectID());
        }

        if (blend_state)
        {
            Utility::Hash::HashCombine(hash, blend_state->GetObjectID());
        }

        if (depth_stencil_state)
        {
            Utility::Hash::HashCombine(hash, depth_stencil_state->GetObjectID());
        }

        // ���̴� ����
        {
            if (shader_compute)
            {
                Utility::Hash::HashCombine(hash, shader_compute->GetObjectID());
            }

            if (shader_vertex)
            {
                Utility::Hash::HashCombine(hash, shader_vertex->GetObjectID());
            }

            if (shader_pixel)
            {
                Utility::Hash::HashCombine(hash, shader_pixel->GetObjectID());
            }
        }

        // ���� Ÿ��
        bool has_rt_color = false;
        {
            uint8_t load_op = 0;

            // ����
            for (uint32_t i = 0; i < rhi_max_render_target_count; i++)
            {
                if (RHI_Texture* texture = render_target_color_textures[i])
                {
                    Utility::Hash::HashCombine(hash, texture->GetObjectID());

                    load_op = clear_color[i] == rhi_color_dont_care ? 0 : clear_color[i] == rhi_color_load ? 1 : 2;
                    Utility::Hash::HashCombine(hash, load_op);

                    has_rt_color = true;
                }
            }

            // ����
            if (render_target_depth_texture)
            {
                Utility::Hash::HashCombine(hash, render_target_depth_texture->GetObjectID());

                load_op = clear_depth == rhi_depth_stencil_dont_care ? 0 : clear_depth == rhi_depth_stencil_load ? 1 : 2;
                Utility::Hash::HashCombine(hash, load_op);

                load_op = clear_stencil == rhi_depth_stencil_dont_care ? 0 : clear_stencil == rhi_depth_stencil_load ? 1 : 2;
                Utility::Hash::HashCombine(hash, load_op);
            }
        }

        return hash;
    }
}