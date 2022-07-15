#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_StructuredBuffer.h"

using namespace std;

namespace PlayGround
{
    RHI_StructuredBuffer::RHI_StructuredBuffer(const shared_ptr<RHI_Device>& rhi_device, const uint32_t stride, const uint32_t element_count, const void* data /*= nullptr*/)
    {
        m_rhi_device = rhi_device;

        D3D11_BUFFER_DESC desc = {};
        {
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.ByteWidth = stride * element_count;
            desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
            desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            desc.StructureByteStride = stride;

            D3D11_SUBRESOURCE_DATA subresource_data = {};
            subresource_data.pSysMem = data;

            if (!d3d11_utility::error_check(m_rhi_device->GetContextRhi()->device->CreateBuffer(&desc, data ? &subresource_data : nullptr, reinterpret_cast<ID3D11Buffer**>(&m_resource))))
                return;
        }

        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
            desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = element_count;

            d3d11_utility::error_check(rhi_device->GetContextRhi()->device->CreateUnorderedAccessView(static_cast<ID3D11Resource*>(m_resource), &desc, reinterpret_cast<ID3D11UnorderedAccessView**>(&m_resource_uav)));
        }
    }

    RHI_StructuredBuffer::~RHI_StructuredBuffer()
    {
        d3d11_utility::release<ID3D11Buffer>(m_resource);
        d3d11_utility::release<ID3D11UnorderedAccessView>(m_resource_uav);
    }

    void* RHI_StructuredBuffer::Map()
    {
        ASSERT(m_rhi_device != nullptr);
        ASSERT(m_rhi_device->GetContextRhi()->device_context != nullptr);
        ASSERT(m_resource != nullptr);

        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        if (!d3d11_utility::error_check(m_rhi_device->GetContextRhi()->device_context->Map(static_cast<ID3D11Resource*>(m_resource), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource)))
        {
            LOG_ERROR("Failed to map structured buffer.");
            return nullptr;
        }

        return mapped_resource.pData;
    }

    void RHI_StructuredBuffer::Unmap()
    {
        ASSERT(m_rhi_device != nullptr);
        ASSERT(m_rhi_device->GetContextRhi()->device_context != nullptr);
        ASSERT(m_resource != nullptr);

        m_rhi_device->GetContextRhi()->device_context->Unmap(static_cast<ID3D11Buffer*>(m_resource), 0);
    }
}