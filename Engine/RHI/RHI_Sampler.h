#pragma once

#include <memory>
#include "../Core/EngineObject.h"
#include "RHI_Definition.h"

namespace PlayGround
{
    // RHI »ùÇÃ·¯
	class RHI_Sampler : public EngineObject
	{
    public:
        RHI_Sampler(
            const std::shared_ptr<RHI_Device>& rhi_device,
            const RHI_Filter filter_min = RHI_Filter::Nearest,
            const RHI_Filter filter_mag = RHI_Filter::Nearest,
            const RHI_Sampler_Mipmap_Mode filter_mipmap = RHI_Sampler_Mipmap_Mode::Nearest,
            const RHI_Sampler_Address_Mode sampler_address_mode = RHI_Sampler_Address_Mode::Wrap,
            const RHI_Comparison_Function comparison_function = RHI_Comparison_Function::Always,
            const float anisotropy = 0.0f,
            const bool comparison_enabled = false,
            const float mip_bias = 0.0f
        );
        ~RHI_Sampler();

        RHI_Filter GetFilterMin()                       const { return m_filter_min; }
        RHI_Filter GetFilterMag()                       const { return m_filter_mag; }
        RHI_Sampler_Mipmap_Mode GetFilterMipmap()       const { return m_filter_mipmap; }
        RHI_Sampler_Address_Mode GetAddressMode()       const { return m_sampler_address_mode; }
        RHI_Comparison_Function GetComparisonFunction() const { return m_comparison_function; }
        bool GetAnisotropyEnabled()                     const { return m_anisotropy != 0; }
        bool GetComparisonEnabled()                     const { return m_comparison_enabled; }
        void* GetResource()                             const { return m_resource; }

    private:
        void CreateResource();

        RHI_Filter m_filter_min = RHI_Filter::Nearest;
        RHI_Filter m_filter_mag = RHI_Filter::Nearest;
        RHI_Sampler_Mipmap_Mode m_filter_mipmap = RHI_Sampler_Mipmap_Mode::Nearest;
        RHI_Sampler_Address_Mode m_sampler_address_mode = RHI_Sampler_Address_Mode::Wrap;
        RHI_Comparison_Function m_comparison_function = RHI_Comparison_Function::Always;
        float m_anisotropy = 0;
        bool m_comparison_enabled = false;
        float m_mip_lod_bias = 0.0f;

        void* m_resource = nullptr;

        std::shared_ptr<RHI_Device> m_rhi_device;
	};
}

