#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_Device.h"
#include "../RHI_IndexBuffer.h"

using namespace std;

namespace PlayGround
{
    void RHI_IndexBuffer::_destroy()
    {
        d3d11_utility::release<ID3D11Buffer>(m_resource);
    }

    bool RHI_IndexBuffer::_create(const void* indices)
    {
        ASSERT(m_rhi_device != nullptr);
        ASSERT(m_rhi_device->GetContextRhi()->device != nullptr);

        const bool is_dynamic = indices == nullptr;

        _destroy();

        D3D11_BUFFER_DESC buffer_desc;
        ZeroMemory(&buffer_desc, sizeof(buffer_desc));
        buffer_desc.ByteWidth = m_stride * m_index_count;
        buffer_desc.Usage = is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
        buffer_desc.CPUAccessFlags = is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
        buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA init_data = {};
        init_data.pSysMem = indices;
        init_data.SysMemPitch = 0;
        init_data.SysMemSlicePitch = 0;

        if (!d3d11_utility::error_check(m_rhi_device->GetContextRhi()->device->CreateBuffer(&buffer_desc, is_dynamic ? nullptr : &init_data, reinterpret_cast<ID3D11Buffer**>(&m_resource))))
        {
            LOG_ERROR(" Failed to create index buffer");
            return false;
        }

        return true;
    }

    void* RHI_IndexBuffer::Map()
    {
        ASSERT(m_rhi_device != nullptr);
        ASSERT(m_rhi_device->GetContextRhi()->device_context != nullptr);
        ASSERT(m_resource != nullptr);

        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        if (!d3d11_utility::error_check(m_rhi_device->GetContextRhi()->device_context->Map(static_cast<ID3D11Resource*>(m_resource), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource)))
        {
            LOG_ERROR("Failed to map index buffer.");
            return nullptr;
        }

        return mapped_resource.pData;
    }

    void RHI_IndexBuffer::Unmap()
    {
        ASSERT(m_resource != nullptr);

        m_rhi_device->GetContextRhi()->device_context->Unmap(static_cast<ID3D11Resource*>(m_resource), 0);
    }
}