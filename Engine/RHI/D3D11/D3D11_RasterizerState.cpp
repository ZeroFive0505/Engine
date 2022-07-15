#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_RasterizerState.h"
#include "../RHI_Device.h"

using namespace std;

namespace PlayGround
{
    RHI_RasterizerState::RHI_RasterizerState
    (
        const shared_ptr<RHI_Device>& rhi_device,
        const RHI_CullMode cull_mode,
        const RHI_PolygonMode polygon_mode,
        const bool depth_clip_enabled,
        const bool scissor_enabled,
        const bool antialised_line_enabled,
        const float depth_bias              /*= 0.0f */,
        const float depth_bias_clamp        /*= 0.0f */,
        const float depth_bias_slope_scaled /*= 0.0f */,
        const float line_width              /*= 1.0f */)
    {
        ASSERT(rhi_device != nullptr);
        ASSERT(rhi_device->GetContextRhi()->device != nullptr);

        m_cull_mode = cull_mode;
        m_polygon_mode = polygon_mode;
        m_depth_clip_enabled = depth_clip_enabled;
        m_scissor_enabled = scissor_enabled;
        m_antialised_line_enabled = antialised_line_enabled;
        m_depth_bias = depth_bias;
        m_depth_bias_clamp = depth_bias_clamp;
        m_depth_bias_slope_scaled = depth_bias_slope_scaled;
        m_line_width = line_width;

        D3D11_RASTERIZER_DESC desc = {};
        desc.CullMode = d3d11_cull_mode[static_cast<uint32_t>(cull_mode)];
        desc.FillMode = d3d11_polygon_mode[static_cast<uint32_t>(polygon_mode)];
        desc.FrontCounterClockwise = false;
        desc.DepthBias = static_cast<UINT>(Math::Util::Floor(depth_bias * (float)(1 << 24)));
        desc.DepthBiasClamp = depth_bias_clamp;
        desc.SlopeScaledDepthBias = depth_bias_slope_scaled;
        desc.DepthClipEnable = depth_clip_enabled;
        desc.MultisampleEnable = false;
        desc.AntialiasedLineEnable = antialised_line_enabled;
        desc.ScissorEnable = scissor_enabled;

        auto rasterizer_state = static_cast<ID3D11RasterizerState*>(m_resource);
        m_initialized = d3d11_utility::error_check(rhi_device->GetContextRhi()->device->CreateRasterizerState(&desc, reinterpret_cast<ID3D11RasterizerState**>(&m_resource)));
    }

    RHI_RasterizerState::~RHI_RasterizerState()
    {
        d3d11_utility::release<ID3D11RasterizerState>(m_resource);
    }
}