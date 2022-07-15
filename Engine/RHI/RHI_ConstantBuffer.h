#pragma once

#include <memory>
#include "../Core/EngineObject.h"

namespace PlayGround
{
	class RHI_ConstantBuffer : public EngineObject
	{
    public:
        RHI_ConstantBuffer(const std::shared_ptr<RHI_Device>& rhi_device, const std::string& name);
        ~RHI_ConstantBuffer() { _destroy(); }

        template<typename T>
        bool Create(const uint32_t element_count = 1)
        {
            m_element_count = element_count;
            m_stride = static_cast<uint64_t>(sizeof(T));
            m_ObjectSizeGPU = static_cast<uint64_t>(m_stride * m_element_count);

            return _create();
        }

        void* Map();
        void Unmap();
        void Flush(const uint64_t size, const uint64_t offset);

        inline void ResetOffset() { m_reset_offset = true; }
        inline bool GetResetOffset()     const { return m_reset_offset; }
        inline bool IsPersistentBuffer() const { return m_persistent_mapping; }
        inline void* GetResource()       const { return m_resource; }
        inline uint64_t GetStride()      const { return m_stride; }
        inline uint64_t GetOffset()      const { return m_offset; }
        inline uint32_t GetStrideCount() const { return m_element_count; }

    private:
        bool _create();
        void _destroy();

        bool m_persistent_mapping = false;
        void* m_mapped_data = nullptr;
        uint64_t m_stride = 0;
        uint32_t m_element_count = 0;
        uint32_t m_offset = 0;
        bool m_reset_offset = true;

        void* m_resource = nullptr;

        std::shared_ptr<RHI_Device> m_rhi_device;
	};
}