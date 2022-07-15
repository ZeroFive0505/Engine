#pragma once

#include "../Core/EngineObject.h"

namespace PlayGround
{
    // RHI 구조화 버퍼
	class RHI_StructuredBuffer : public EngineObject
	{
    public:
        RHI_StructuredBuffer(const std::shared_ptr<RHI_Device>& rhi_device, const uint32_t stride, const uint32_t element_count, const void* data = nullptr);
        ~RHI_StructuredBuffer();

        void* Map();
        void Unmap();

        inline void* GetResource() { return m_resource; }
        inline void* GetResourceUav() { return m_resource_uav; }

    private:
        std::shared_ptr<RHI_Device> m_rhi_device;
        void* m_resource = nullptr;
        void* m_resource_uav = nullptr;
        uint32_t m_stride = 0;
        uint32_t m_element_count = 0;
	};
}