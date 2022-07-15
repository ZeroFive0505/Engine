#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_DepthStencilState.h"
#include "../RHI_Device.h"

using namespace std;

namespace PlayGround
{
    RHI_DepthStencilState::RHI_DepthStencilState(
        const shared_ptr<RHI_Device>& rhi_device,
        const bool depth_test                                     /*= true*/,
        const bool depth_write                                    /*= true*/,
        const RHI_Comparison_Function depth_comparison_function   /*= Comparison_LessEqual*/,
        const bool stencil_test                                   /*= false */,
        const bool stencil_write                                  /*= false */,
        const RHI_Comparison_Function stencil_comparison_function /*= RHI_Comparison_Equal */,
        const RHI_Stencil_Operation stencil_fail_op               /*= RHI_Stencil_Keep */,
        const RHI_Stencil_Operation stencil_depth_fail_op         /*= RHI_Stencil_Keep */,
        const RHI_Stencil_Operation stencil_pass_op               /*= RHI_Stencil_Replace */
    )
    {
        ASSERT(rhi_device != nullptr);
        ASSERT(rhi_device->GetContextRhi()->device != nullptr);

        m_depth_test_enabled = depth_test;
        m_depth_write_enabled = depth_write;
        m_depth_comparison_function = depth_comparison_function;
        m_stencil_test_enabled = stencil_test;
        m_stencil_write_enabled = stencil_write;
        m_stencil_comparison_function = stencil_comparison_function;
        m_stencil_fail_op = stencil_fail_op;
        m_stencil_depth_fail_op = stencil_depth_fail_op;
        m_stencil_pass_op = stencil_pass_op;

        D3D11_DEPTH_STENCIL_DESC desc;
        {
            desc.DepthEnable = static_cast<BOOL>(depth_test || depth_write);
            desc.DepthWriteMask = depth_write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc = d3d11_comparison_function[static_cast<uint32_t>(depth_comparison_function)];
            desc.StencilEnable = static_cast<BOOL>(stencil_test || stencil_write);
            desc.StencilReadMask = stencil_test ? GetStencilReadMask() : 0;
            desc.StencilWriteMask = stencil_write ? GetStencilWriteMask() : 0;
            desc.FrontFace.StencilFailOp = d3d11_stencil_operation[static_cast<uint32_t>(m_stencil_fail_op)];
            desc.FrontFace.StencilDepthFailOp = d3d11_stencil_operation[static_cast<uint32_t>(m_stencil_depth_fail_op)];
            desc.FrontFace.StencilPassOp = d3d11_stencil_operation[static_cast<uint32_t>(m_stencil_pass_op)];
            desc.FrontFace.StencilFunc = d3d11_comparison_function[static_cast<uint32_t>(stencil_comparison_function)];
            desc.BackFace = desc.FrontFace;
        }

        auto depth_stencil_state = static_cast<ID3D11DepthStencilState*>(m_resource);
        m_initialized = d3d11_utility::error_check(rhi_device->GetContextRhi()->device->CreateDepthStencilState(&desc, reinterpret_cast<ID3D11DepthStencilState**>(&m_resource)));
    }

    RHI_DepthStencilState::~RHI_DepthStencilState()
    {
        d3d11_utility::release<ID3D11DepthStencilState>(m_resource);
    }
}