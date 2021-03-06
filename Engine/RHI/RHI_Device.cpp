#include "Common.h"
#include "RHI_Device.h"
#include "RHI_Implementation.h"
#include "RHI_CommandPool.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
    void RHI_Device::RegisterPhysicalDevice(const PhysicalDevice& physical_device)
    {
        m_physical_devices.emplace_back(physical_device);

        sort(m_physical_devices.begin(), m_physical_devices.end(), [](const PhysicalDevice& adapter1, const PhysicalDevice& adapter2)
        {
            return adapter1.GetMemory() > adapter2.GetMemory();
        });

        LOG_INFO("%s (%d MB)", physical_device.GetName().c_str(), physical_device.GetMemory());
    }

    const PhysicalDevice* RHI_Device::GetPrimaryPhysicalDevice()
    {
        if (m_physical_device_index >= m_physical_devices.size())
            return nullptr;

        return &m_physical_devices[m_physical_device_index];
    }

    void RHI_Device::SetPrimaryPhysicalDevice(const uint32_t index)
    {
        m_physical_device_index = index;

        if (const PhysicalDevice* physical_device = GetPrimaryPhysicalDevice())
        {
            LOG_INFO("%s (%d MB)", physical_device->GetName().c_str(), physical_device->GetMemory());
        }
    }

    RHI_CommandPool* RHI_Device::AllocateCommandPool(const char* name, const uint64_t swap_chain_id)
    {
        return m_cmd_pools.emplace_back(make_shared<RHI_CommandPool>(this, name, swap_chain_id)).get();
    }

    bool RHI_Device::IsValidResolution(const uint32_t width, const uint32_t height)
    {
        return width > 4 && width <= m_max_texture_2d_dimension &&
            height > 4 && height <= m_max_texture_2d_dimension;
    }

    bool RHI_Device::QueueWaitAll() const
    {
        return QueueWait(RHI_Queue_Type::Graphics) && QueueWait(RHI_Queue_Type::Copy) && QueueWait(RHI_Queue_Type::Compute);
    }

    void* RHI_Device::GetQueue(const RHI_Queue_Type type) const
    {
        if (type == RHI_Queue_Type::Graphics)
        {
            return m_queue_graphics;
        }
        else if (type == RHI_Queue_Type::Copy)
        {
            return m_queue_copy;
        }
        else if (type == RHI_Queue_Type::Compute)
        {
            return m_queue_compute;
        }

        return nullptr;
    }

    uint32_t RHI_Device::GetQueueIndex(const RHI_Queue_Type type) const
    {
        if (type == RHI_Queue_Type::Graphics)
        {
            return m_queue_graphics_index;
        }
        else if (type == RHI_Queue_Type::Copy)
        {
            return m_queue_copy_index;
        }
        else if (type == RHI_Queue_Type::Compute)
        {
            return m_queue_compute_index;
        }

        return 0;
    }

    void RHI_Device::SetQueueIndex(const RHI_Queue_Type type, const uint32_t index)
    {
        if (type == RHI_Queue_Type::Graphics)
        {
            m_queue_graphics_index = index;
        }
        else if (type == RHI_Queue_Type::Copy)
        {
            m_queue_copy_index = index;
        }
        else if (type == RHI_Queue_Type::Compute)
        {
            m_queue_compute_index = index;
        }
    }

    bool RHI_Device::HasDescriptorSetCapacity()
    {
        const uint32_t required_capacity = static_cast<uint32_t>(m_descriptor_sets.size());
        return m_descriptor_set_capacity > required_capacity;
    }
}