#pragma once

#include "RHI_Definition.h"
#include "RHI_Viewport.h"
#include "../Core/EngineObject.h"
#include "../Math/Rectangle.h"
#include <array>

namespace PlayGround
{
	class RHI_PipelineState : public EngineObject
	{
    public:
        RHI_PipelineState();
        ~RHI_PipelineState();

        bool IsValid();
        uint32_t GetWidth() const;
        uint32_t GetHeight() const;
        uint32_t ComputeHash() const;
        bool HasClearValues();
        inline bool IsGraphics() const { return (shader_vertex != nullptr || shader_pixel != nullptr) && !shader_compute; }
        inline bool IsCompute()  const { return shader_compute != nullptr && !IsGraphics(); }

        RHI_Shader* shader_vertex = nullptr;
        RHI_Shader* shader_pixel = nullptr;
        RHI_Shader* shader_compute = nullptr;
        RHI_RasterizerState* rasterizer_state = nullptr;
        RHI_BlendState* blend_state = nullptr;
        RHI_DepthStencilState* depth_stencil_state = nullptr;
        RHI_SwapChain* render_target_swapchain = nullptr;
        RHI_PrimitiveTopology_Mode primitive_topology = RHI_PrimitiveTopology_Mode::Undefined;
        RHI_Viewport viewport = RHI_Viewport::Undefined;
        Math::Rectangle scissor = Math::Rectangle::Zero;
        bool dynamic_scissor = false;
        bool can_use_vertex_index_buffers = true;

        RHI_Texture* render_target_depth_texture = nullptr;
        std::array<RHI_Texture*, rhi_max_render_target_count> render_target_color_textures =
        {
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr
        };

        uint32_t render_target_color_texture_array_index = 0;
        uint32_t render_target_depth_stencil_texture_array_index = 0;

        float clear_depth = rhi_depth_stencil_load;
        float clear_stencil = rhi_depth_stencil_load;
        std::array<Math::Vector4, rhi_max_render_target_count> clear_color;

        bool render_target_depth_texture_read_only = false;

        std::array<int, rhi_max_constant_buffer_count> dynamic_constant_buffer_slots =
        {
            0, 1, 2, 3, 4, -1, -1, -1
        };

        const char* pass_name = nullptr;
        bool gpu_marker = true;
        bool profile = true;

    private:
        const RHI_Device* m_rhi_device = nullptr;
	};
}

