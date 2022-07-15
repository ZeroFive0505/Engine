#pragma once

#include "RHI_Definition.h"
#include "../Core/EngineObject.h"

namespace PlayGround
{
    // RHI 블렌드 스테이트 클래스
	class RHI_BlendState : public EngineObject
	{
    public:
        RHI_BlendState() = default;
        RHI_BlendState(
            const std::shared_ptr<RHI_Device>& device,
            const bool blend_enabled = false,
            const RHI_Blend source_blend = RHI_Blend::Src_Alpha,
            const RHI_Blend dest_blend = RHI_Blend::Inv_Src_Alpha,
            const RHI_Blend_Operation blend_op = RHI_Blend_Operation::Add,
            const RHI_Blend source_blend_alpha = RHI_Blend::One,
            const RHI_Blend dest_blend_alpha = RHI_Blend::One,
            const RHI_Blend_Operation blend_op_alpha = RHI_Blend_Operation::Add,
            const float blend_factor = 0.0f
        );
        ~RHI_BlendState();

        inline auto GetBlendEnabled()                        const { return m_blend_enabled; }
        inline auto GetSourceBlend()                         const { return m_source_blend; }
        inline auto GetDestBlend()                           const { return m_dest_blend; }
        inline auto GetBlendOp()                             const { return m_blend_op; }
        inline auto GetSourceBlendAlpha()                    const { return m_source_blend_alpha; }
        inline auto GetDestBlendAlpha()                      const { return m_dest_blend_alpha; }
        inline auto GetBlendOpAlpha()                        const { return m_blend_op_alpha; }
        inline auto GetResource()                            const { return m_resource; }
        inline void SetBlendFactor(const float blend_factor) { m_blend_factor = blend_factor; }
        inline float GetBlendFactor()                        const { return m_blend_factor; }

        bool operator==(const RHI_BlendState& rhs) const
        {
            return
                m_blend_enabled == rhs.GetBlendEnabled() &&
                m_source_blend == rhs.GetSourceBlend() &&
                m_dest_blend == rhs.GetDestBlend() &&
                m_blend_op == rhs.GetBlendOp() &&
                m_source_blend_alpha == rhs.GetSourceBlendAlpha() &&
                m_dest_blend_alpha == rhs.GetDestBlendAlpha() &&
                m_blend_op_alpha == rhs.GetBlendOpAlpha();
        }

    private:
        bool m_blend_enabled = false;
        RHI_Blend m_source_blend = RHI_Blend::Src_Alpha;
        RHI_Blend m_dest_blend = RHI_Blend::Inv_Src_Alpha;
        RHI_Blend_Operation m_blend_op = RHI_Blend_Operation::Add;
        RHI_Blend m_source_blend_alpha = RHI_Blend::One;
        RHI_Blend m_dest_blend_alpha = RHI_Blend::One;
        RHI_Blend_Operation m_blend_op_alpha = RHI_Blend_Operation::Add;
        float m_blend_factor = 1.0f;

        void* m_resource = nullptr;
        bool m_initialized = false;
	};
}