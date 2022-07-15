#pragma once

#include <memory>
#include "RHI_Definition.h"
#include "../Core/EngineObject.h"

namespace PlayGround
{
    // RHI 래스터라이저 스테이트
	class RHI_RasterizerState : public EngineObject
	{
    public:
        RHI_RasterizerState() = default;
        RHI_RasterizerState(
            const std::shared_ptr<RHI_Device>& rhi_device,
            const RHI_CullMode cull_mode,
            const RHI_PolygonMode fill_mode,
            const bool depth_clip_enabled,
            const bool scissor_enabled,
            const bool antialised_line_enabled,
            const float depth_bias = 0.0f,
            const float depth_bias_clamp = 0.0f,
            const float depth_bias_slope_scaled = 0.0f,
            const float line_width = 1.0f
        );
        ~RHI_RasterizerState();

        inline RHI_CullMode GetCullMode()     const { return m_cull_mode; }
        inline RHI_PolygonMode GetPolygonMode()  const { return m_polygon_mode; }
        inline bool GetDepthClipEnabled()      const { return m_depth_clip_enabled; }
        inline bool GetScissorEnabled()        const { return m_scissor_enabled; }
        inline bool GetAntialisedLineEnabled() const { return m_antialised_line_enabled; }
        inline bool IsInitialized()            const { return m_initialized; }
        inline void* GetResource()             const { return m_resource; }
        inline float GetLineWidth()            const { return m_line_width; }
        inline float GetDepthBias()            const { return m_depth_bias; }
        inline float GetDepthBiasClamp()       const { return m_depth_bias_clamp; }
        inline float GetDepthBiasSlopeScaled() const { return m_depth_bias_slope_scaled; }

        bool operator==(const RHI_RasterizerState& rhs) const
        {
            return
                m_cull_mode == rhs.GetCullMode() &&
                m_polygon_mode == rhs.GetPolygonMode() &&
                m_depth_clip_enabled == rhs.GetDepthClipEnabled() &&
                m_scissor_enabled == rhs.GetScissorEnabled() &&
                m_antialised_line_enabled == rhs.GetAntialisedLineEnabled() &&
                m_depth_bias == rhs.GetDepthBias() &&
                m_depth_bias_slope_scaled == rhs.GetDepthBiasSlopeScaled() &&
                m_line_width == rhs.GetLineWidth();
        }

    private:
        RHI_CullMode m_cull_mode = RHI_CullMode::Undefined;
        RHI_PolygonMode m_polygon_mode = RHI_PolygonMode::Undefined;
        bool m_depth_clip_enabled = false;
        bool m_scissor_enabled = false;
        bool m_antialised_line_enabled = false;
        float m_depth_bias = 0.0f;
        float m_depth_bias_clamp = 0.0f;
        float m_depth_bias_slope_scaled = 0.0f;
        float m_line_width = 1.0f;

        bool m_initialized = false;

        void* m_resource = nullptr;
	};
}