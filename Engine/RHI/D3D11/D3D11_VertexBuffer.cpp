#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_Device.h"
#include "../RHI_VertexBuffer.h"

using namespace std;

namespace PlayGround
{
    void RHI_VertexBuffer::_destroy()
    {
        d3d11_utility::release<ID3D11Buffer>(m_resource);
    }

    bool RHI_VertexBuffer::_create(const void* vertices)
    {
        ASSERT(m_rhi_device != nullptr);
        ASSERT(m_rhi_device->GetContextRhi()->device_context != nullptr);

        const bool is_dynamic = vertices == nullptr;

        _destroy();

        D3D11_BUFFER_DESC buffer_desc = {};
        buffer_desc.ByteWidth = static_cast<UINT>(m_ObjectSizeGPU);
        buffer_desc.Usage = is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
        buffer_desc.CPUAccessFlags = is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_desc.MiscFlags = 0;
        buffer_desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA init_data = {};
        init_data.pSysMem = vertices;
        init_data.SysMemPitch = 0;
        init_data.SysMemSlicePitch = 0;

        const auto ptr = reinterpret_cast<ID3D11Buffer**>(&m_resource);
        const auto result = m_rhi_device->GetContextRhi()->device->CreateBuffer(&buffer_desc, is_dynamic ? nullptr : &init_data, ptr);
        if (FAILED(result))
        {
            LOG_ERROR("Failed to create vertex buffer");
            return false;
        }

        return true;
    }

    void* RHI_VertexBuffer::Map()
    {
        ASSERT(m_resource != nullptr);

        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        const auto result = m_rhi_device->GetContextRhi()->device_context->Map(static_cast<ID3D11Resource*>(m_resource), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
        if (FAILED(result))
        {
            LOG_ERROR("Failed to map vertex buffer");
            return nullptr;
        }

        return mapped_resource.pData;
    }

    void RHI_VertexBuffer::Unmap()
    {
        ASSERT(m_resource != nullptr);

        m_rhi_device->GetContextRhi()->device_context->Unmap(static_cast<ID3D11Resource*>(m_resource), 0);
    }
}