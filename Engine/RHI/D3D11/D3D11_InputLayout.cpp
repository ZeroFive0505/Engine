#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_InputLayout.h"
#include "../RHI_Device.h"

using namespace std;

namespace PlayGround
{
    RHI_InputLayout::~RHI_InputLayout()
    {
        d3d11_utility::release<ID3D11InputLayout>(m_resource);
    }

    bool RHI_InputLayout::_CreateResource(void* vertex_shader_blob)
    {
        ASSERT(vertex_shader_blob != nullptr);
        ASSERT(!m_vertex_attributes.empty());

        vector<D3D11_INPUT_ELEMENT_DESC> vertex_attributes;
        for (const auto& vertex_attribute : m_vertex_attributes)
        {
            vertex_attributes.emplace_back(D3D11_INPUT_ELEMENT_DESC
                {
                    vertex_attribute.name.c_str(),          // SemanticName
                    0,                                      // SemanticIndex
                    d3d11_format[vertex_attribute.format],  // Format
                    0,                                      // InputSlot
                    vertex_attribute.offset,                // AlignedByteOffset
                    D3D11_INPUT_PER_VERTEX_DATA,            // InputSlotClass
                    0                                       // InstanceDataStepRate
                });
        }

        auto d3d_blob = static_cast<ID3D10Blob*>(vertex_shader_blob);
        const auto result = m_rhi_device->GetContextRhi()->device->CreateInputLayout
        (
            vertex_attributes.data(),
            static_cast<UINT>(vertex_attributes.size()),
            d3d_blob->GetBufferPointer(),
            d3d_blob->GetBufferSize(),
            reinterpret_cast<ID3D11InputLayout**>(&m_resource)
        );

        if (FAILED(result))
        {
            LOG_ERROR("Failed to create input layout, %s", d3d11_utility::dxgi_error_to_string(result));
            return false;
        }

        return true;
    }
}