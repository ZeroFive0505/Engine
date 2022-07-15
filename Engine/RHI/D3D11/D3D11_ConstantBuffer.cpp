#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_ConstantBuffer.h"
#include "../RHI_Device.h"
#include "../../Log/Logger.h"

using namespace std;

namespace PlayGround
{
    bool RHI_ConstantBuffer::_create()
    {
        ASSERT(m_rhi_device != nullptr);
        ASSERT(m_rhi_device->GetContextRhi()->device != nullptr);

        _destroy();

        D3D11_BUFFER_DESC buffer_desc;
        ZeroMemory(&buffer_desc, sizeof(buffer_desc));
        buffer_desc.ByteWidth = static_cast<UINT>(m_stride);
        buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;

        const auto result = m_rhi_device->GetContextRhi()->device->CreateBuffer(&buffer_desc, nullptr, reinterpret_cast<ID3D11Buffer**>(&m_resource));
        if (FAILED(result))
        {
            LOG_ERROR("Failed to create constant buffer");
            return false;
        }

        return true;
    }

    void RHI_ConstantBuffer::_destroy()
    {
        d3d11_utility::release<ID3D11Buffer>(m_resource);
    }

    RHI_ConstantBuffer::RHI_ConstantBuffer(const std::shared_ptr<RHI_Device>& rhi_device, const string& name)
    {
        m_rhi_device = rhi_device;
        m_ObjectName = name;
    }

    void* RHI_ConstantBuffer::Map()
    {
        ASSERT(m_rhi_device != nullptr);
        ASSERT(m_rhi_device->GetContextRhi()->device_context != nullptr);
        ASSERT(m_resource != nullptr);

        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        const auto result = m_rhi_device->GetContextRhi()->device_context->Map(static_cast<ID3D11Buffer*>(m_resource), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
        if (FAILED(result))
        {
            LOG_ERROR("Failed to map constant buffer.");
            return nullptr;
        }

        return mapped_resource.pData;
    }

    void RHI_ConstantBuffer::Unmap()
    {
        ASSERT(m_resource != nullptr);
        m_rhi_device->GetContextRhi()->device_context->Unmap(static_cast<ID3D11Buffer*>(m_resource), 0);
    }

    void RHI_ConstantBuffer::Flush(const uint64_t size, const uint64_t offset)
    {

    }
}