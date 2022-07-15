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
        // 파이프라인 상태
        bool has_shader_compute = shader_compute ? shader_compute->IsCompiled() : false;
        bool has_shader_vertex = shader_vertex ? shader_vertex->IsCompiled() : false;
        bool has_shader_pixel = shader_pixel ? shader_pixel->IsCompiled() : false;
        bool has_render_target = render_target_color_textures[0] || render_target_depth_texture; // 렌더 타겟이 설정이 되어 있는지
        bool has_backbuffer = render_target_swapchain;                                        // 백버퍼 여부
        bool has_graphics_states = rasterizer_state && blend_state && depth_stencil_state && primitive_topology != RHI_PrimitiveTopology_Mode::Undefined;
        bool is_graphics_pso = (has_shader_vertex || has_shader_pixel) && !has_shader_compute;
        bool is_compute_pso = has_shader_compute && (!has_shader_vertex && !has_shader_pixel);

        // 파이프라인에 바인딩된 쉐이더가 없을 수가 있다(상수 버퍼 데이터만 업데이할시에)

        // 만약 그래픽 파이프라인이지만 제대로된 스테이트가 설정되지 않았으면 반환
        if (is_graphics_pso && !has_graphics_states)
        {
            return false;
        }

        // 렌더 타겟이 설정되어 있는지 않다면
        if (is_graphics_pso && !has_render_target && !has_backbuffer)
        {
            // 둘다 바인드 되어있지 않아도 반환
            if (!has_render_target && !has_backbuffer)
            {
                return false;
            }

            // 둘다 바인드 되어있어도 반환
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
        // 파이프라인 고유 해시 번호를 부여한다.

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

        // 쉐이더 여부
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

        // 렌더 타겟
        bool has_rt_color = false;
        {
            uint8_t load_op = 0;

            // 색깔
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

            // 깊이
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