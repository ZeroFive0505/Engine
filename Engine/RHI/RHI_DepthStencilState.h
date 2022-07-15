#pragma once

#include <memory>
#include "RHI_Definition.h"
#include "../Core/EngineObject.h"

namespace PlayGround
{
    // RHI ±Ì¿Ã, Ω∫≈ŸΩ«
	class RHI_DepthStencilState : public EngineObject
	{
    public:
        RHI_DepthStencilState() = default;
        RHI_DepthStencilState(
            const std::shared_ptr<RHI_Device>& rhi_device,
            const bool depth_test = true,
            const bool depth_write = true,
            const RHI_Comparison_Function depth_comparison_function = RHI_Comparison_Function::LessEqual,
            const bool stencil_test = false,
            const bool stencil_write = false,
            const RHI_Comparison_Function stencil_comparison_function = RHI_Comparison_Function::Always,
            const RHI_Stencil_Operation stencil_fail_op = RHI_Stencil_Operation::Keep,
            const RHI_Stencil_Operation stencil_depth_fail_op = RHI_Stencil_Operation::Keep,
            const RHI_Stencil_Operation stencil_pass_op = RHI_Stencil_Operation::Replace
        );
        ~RHI_DepthStencilState();

        inline bool GetDepthTestEnabled()                             const { return m_depth_test_enabled; }
        inline bool GetDepthWriteEnabled()                            const { return m_depth_write_enabled; }
        inline bool GetStencilTestEnabled()                           const { return m_stencil_test_enabled; }
        inline bool GetStencilWriteEnabled()                          const { return m_stencil_write_enabled; }
        inline RHI_Comparison_Function GetDepthComparisonFunction()   const { return m_depth_comparison_function; }
        inline RHI_Comparison_Function GetStencilComparisonFunction() const { return m_stencil_comparison_function; }
        inline RHI_Stencil_Operation GetStencilFailOperation()        const { return m_stencil_fail_op; }
        inline RHI_Stencil_Operation GetStencilDepthFailOperation()   const { return m_stencil_depth_fail_op; }
        inline RHI_Stencil_Operation GetStencilPassOperation()        const { return m_stencil_pass_op; }
        inline uint8_t GetStencilReadMask()                           const { return m_stencil_read_mask; }
        inline uint8_t GetStencilWriteMask()                          const { return m_stencil_write_mask; }
        inline void* GetResource()                                    const { return m_resource; }

    private:
        bool m_depth_test_enabled = false;
        bool m_depth_write_enabled = false;
        RHI_Comparison_Function m_depth_comparison_function = RHI_Comparison_Function::Never;
        bool m_stencil_test_enabled = false;
        bool m_stencil_write_enabled = false;
        RHI_Comparison_Function m_stencil_comparison_function = RHI_Comparison_Function::Never;
        RHI_Stencil_Operation m_stencil_fail_op = RHI_Stencil_Operation::Keep;
        RHI_Stencil_Operation m_stencil_depth_fail_op = RHI_Stencil_Operation::Keep;
        RHI_Stencil_Operation m_stencil_pass_op = RHI_Stencil_Operation::Replace;
        uint8_t m_stencil_read_mask = 1;
        uint8_t m_stencil_write_mask = 1;
        bool m_initialized = false;
        void* m_resource = nullptr;
	};
}