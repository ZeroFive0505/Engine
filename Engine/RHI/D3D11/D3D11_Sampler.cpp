#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_Sampler.h"
#include "../RHI_Device.h"

namespace PlayGround
{
    void RHI_Sampler::CreateResource()
    {
        D3D11_SAMPLER_DESC sampler_desc = {};
        sampler_desc.Filter = d3d11_utility::sampler::get_filter(m_filter_min, m_filter_mag, m_filter_mipmap, m_anisotropy != 0, m_comparison_enabled);
        sampler_desc.AddressU = d3d11_sampler_address_mode[static_cast<uint32_t>(m_sampler_address_mode)];
        sampler_desc.AddressV = d3d11_sampler_address_mode[static_cast<uint32_t>(m_sampler_address_mode)];
        sampler_desc.AddressW = d3d11_sampler_address_mode[static_cast<uint32_t>(m_sampler_address_mode)];
        sampler_desc.MipLODBias = m_mip_lod_bias;
        sampler_desc.MaxAnisotropy = static_cast<UINT>(m_anisotropy);
        sampler_desc.ComparisonFunc = d3d11_comparison_function[static_cast<uint32_t>(m_comparison_function)];
        sampler_desc.BorderColor[0] = 0;
        sampler_desc.BorderColor[1] = 0;
        sampler_desc.BorderColor[2] = 0;
        sampler_desc.BorderColor[3] = 0;
        sampler_desc.MinLOD = 0;
        sampler_desc.MaxLOD = FLT_MAX;

        d3d11_utility::error_check(m_rhi_device->GetContextRhi()->device->CreateSamplerState(&sampler_desc, reinterpret_cast<ID3D11SamplerState**>(&m_resource)));
    }

    RHI_Sampler::~RHI_Sampler()
    {
        d3d11_utility::release<ID3D11SamplerState>(m_resource);
    }
}