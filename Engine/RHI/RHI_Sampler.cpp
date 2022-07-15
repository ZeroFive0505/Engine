#include "Common.h"
#include "RHI_Sampler.h"
#include "RHI_Device.h"
#include "RHI_Implementation.h"

namespace PlayGround
{
    RHI_Sampler::RHI_Sampler(
        const std::shared_ptr<RHI_Device>& rhi_device,
        const RHI_Filter filter_min                         /* = RHI_Filter_Nearest */,
        const RHI_Filter filter_mag                         /* = RHI_Filter_Nearest */,
        const RHI_Sampler_Mipmap_Mode filter_mipmap         /* = RHI_Sampler_Mipmap_Nearest */,
        const RHI_Sampler_Address_Mode sampler_address_mode /* = RHI_Sampler_Address_Wrap */,
        const RHI_Comparison_Function comparison_function   /* = RHI_Comparison_Always */,
        const float anisotropy                              /* = 0.0f */,
        const bool comparison_enabled                       /* = false */,
        const float mip_lod_bias                            /* = 0.0f */
    )
    {
        ASSERT(rhi_device != nullptr);
        ASSERT(rhi_device->GetContextRhi()->device != nullptr);

        m_resource = nullptr;
        m_rhi_device = rhi_device;
        m_filter_min = filter_min;
        m_filter_mag = filter_mag;
        m_filter_mipmap = filter_mipmap;
        m_sampler_address_mode = sampler_address_mode;
        m_comparison_function = comparison_function;
        m_anisotropy = anisotropy;
        m_comparison_enabled = comparison_enabled;
        m_mip_lod_bias = mip_lod_bias;

        CreateResource();
    }
}