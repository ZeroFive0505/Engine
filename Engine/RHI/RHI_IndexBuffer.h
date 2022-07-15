#pragma once

#include <vector>
#include "../Core/EngineObject.h"

namespace PlayGround
{
    // RHI ÀÎµ¦½º ¹öÆÛ
	class RHI_IndexBuffer : public EngineObject
	{
    public:
        RHI_IndexBuffer(const std::shared_ptr<RHI_Device>& rhi_device, bool is_mappable, const char* name)
        {
            m_rhi_device = rhi_device;
            m_is_mappable = is_mappable;
            m_ObjectName = name;
        }

        ~RHI_IndexBuffer()
        {
            _destroy();
        }

        template<typename T>
        bool Create(const std::vector<T>& indices)
        {
            m_stride = sizeof(T);
            m_index_count = static_cast<uint32_t>(indices.size());
            m_ObjectSizeGPU = static_cast<uint64_t>(m_stride * m_index_count);
            return _create(static_cast<const void*>(indices.data()));
        }

        template<typename T>
        bool Create(const T* indices, const uint32_t index_count)
        {
            m_stride = sizeof(T);
            m_index_count = index_count;
            m_ObjectSizeGPU = static_cast<uint64_t>(m_stride * m_index_count);
            return _create(static_cast<const void*>(indices));
        }

        template<typename T>
        bool CreateDynamic(const uint32_t index_count)
        {
            m_stride = sizeof(T);
            m_index_count = index_count;
            m_ObjectSizeGPU = static_cast<uint64_t>(m_stride * m_index_count);
            return _create(nullptr);
        }

        void* Map();
        void Unmap();

        inline void* GetResource()      const { return m_resource; }
        inline uint32_t GetIndexCount() const { return m_index_count; }
        inline bool Is16Bit()           const { return sizeof(uint16_t) == m_stride; }
        inline bool Is32Bit()           const { return sizeof(uint32_t) == m_stride; }

    private:
        bool _create(const void* indices);
        void _destroy();

        void* m_mapped_data = nullptr;
        uint32_t m_stride = 0;
        uint32_t m_index_count = 0;

        std::shared_ptr<RHI_Device> m_rhi_device;
        void* m_resource = nullptr;
        bool m_is_mappable = false;
	};
}