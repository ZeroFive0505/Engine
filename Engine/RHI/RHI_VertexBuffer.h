#pragma once

#include <vector>
#include "../Core/EngineObject.h"

#include <memory>

namespace PlayGround
{
	class RHI_Device;

    // RHI 버텍스 버퍼
	class RHI_VertexBuffer : public EngineObject
	{
    public:
        RHI_VertexBuffer(const std::shared_ptr<RHI_Device>& rhi_device, bool is_mappable, const char* name)
        {
            m_rhi_device = rhi_device;
            m_is_mappable = is_mappable;
            m_ObjectName = name;
        }

        ~RHI_VertexBuffer()
        {
            _destroy();
        }

        template<typename T>
        bool Create(const std::vector<T>& vertices)
        {
            m_stride = static_cast<uint32_t>(sizeof(T));
            m_vertex_count = static_cast<uint32_t>(vertices.size());
            m_ObjectSizeGPU = static_cast<uint64_t>(m_stride * m_vertex_count);
            return _create(static_cast<const void*>(vertices.data()));
        }

        template<typename T>
        bool Create(const T* vertices, const uint32_t vertex_count)
        {
            m_stride = static_cast<uint32_t>(sizeof(T));
            m_vertex_count = vertex_count;
            m_ObjectSizeGPU = static_cast<uint64_t>(m_stride * m_vertex_count);
            return _create(static_cast<const void*>(vertices));
        }

        template<typename T>
        bool CreateDynamic(const uint32_t vertex_count)
        {
            m_stride = static_cast<uint32_t>(sizeof(T));
            m_vertex_count = vertex_count;
            m_ObjectSizeGPU = static_cast<uint64_t>(m_stride * m_vertex_count);
            return _create(nullptr);
        }

        void* Map();
        void Unmap();

        inline void* GetResource()       const { return m_resource; }
        inline uint32_t GetStride()      const { return m_stride; }
        inline uint32_t GetVertexCount() const { return m_vertex_count; }

    private:
        bool _create(const void* vertices);
        void _destroy();

        void* m_mapped_data = nullptr;
        uint32_t m_stride = 0;
        uint32_t m_vertex_count = 0;

        // API
        std::shared_ptr<RHI_Device> m_rhi_device;
        void* m_resource = nullptr;
        bool m_is_mappable = false;
	};
}