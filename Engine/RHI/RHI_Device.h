#pragma once

#include "../Core/EngineObject.h"
#include "../Display/DisplayMode.h"
#include <mutex>
#include <memory>
#include "RHI_PhysicalDevice.h"
#include "RHI_DescriptorSet.h"

#include <unordered_map>

namespace PlayGround
{
    // RHI 디바이스
	class RHI_Device : public EngineObject
	{
    public:
        RHI_Device(Context* context);
        ~RHI_Device();

        const PhysicalDevice* GetPrimaryPhysicalDevice();

        bool QueuePresent(void* swapchain_view, uint32_t* image_index, std::vector<RHI_Semaphore*>& wait_semaphores) const;
        bool QueueSubmit(const RHI_Queue_Type type, const uint32_t wait_flags, void* cmd_buffer, RHI_Semaphore* wait_semaphore = nullptr, RHI_Semaphore* signal_semaphore = nullptr, RHI_Fence* signal_fence = nullptr) const;
        bool QueueWait(const RHI_Queue_Type type) const;
        bool QueueWaitAll() const;
        void* GetQueue(const RHI_Queue_Type type) const;
        uint32_t GetQueueIndex(const RHI_Queue_Type type) const;
        void SetQueueIndex(const RHI_Queue_Type type, const uint32_t index);

        void QueryCreate(void** query = nullptr, RHI_Query_Type type = RHI_Query_Type::Timestamp);
        void QueryRelease(void*& query);
        void QueryBegin(void* query);
        void QueryEnd(void* query);
        void QueryGetData(void* query);

        inline uint32_t GetMaxTexture1dDimension()            const { return m_max_texture_1d_dimension; }
        inline uint32_t GetMaxTexture2dDimension()            const { return m_max_texture_2d_dimension; }
        inline uint32_t GetMaxTexture3dDimension()            const { return m_max_texture_3d_dimension; }
        inline uint32_t GetMaxTextureCubeDimension()          const { return m_max_texture_cube_dimension; }
        inline uint32_t GetMaxTextureArrayLayers()            const { return m_max_texture_array_layers; }
        inline uint64_t GetMinUniformBufferOffsetAllignment() const { return m_min_uniform_buffer_offset_alignment; }
        inline float GetTimestampPeriod()                     const { return m_timestamp_period; }

        inline void* GetDescriptorPool() { return m_descriptor_pool; }
        inline std::unordered_map<uint32_t, RHI_DescriptorSet>& GetDescriptorSets() { return m_descriptor_sets; }
        bool HasDescriptorSetCapacity();
        void SetDescriptorSetCapacity(uint32_t descriptor_set_capacity);

        RHI_CommandPool* AllocateCommandPool(const char* name, const uint64_t swap_chain_id);
        inline const std::vector<std::shared_ptr<RHI_CommandPool>>& GetCommandPools() { return m_cmd_pools; }

        bool IsValidResolution(const uint32_t width, const uint32_t height);
        inline RHI_Context* GetContextRhi()        const { return m_rhi_context.get(); }
        inline Context* GetContext()               const { return m_Context; }
        inline uint32_t GetEnabledGraphicsStages() const { return m_enabled_graphics_shader_stages; }

    private:
        bool DetectPhysicalDevices();
        void RegisterPhysicalDevice(const PhysicalDevice& physical_device);
        bool SelectPrimaryPhysicalDevice();
        void SetPrimaryPhysicalDevice(const uint32_t index);

        bool DetectDisplayModes(const PhysicalDevice* physical_device, const RHI_Format format);

        void* m_queue_graphics = nullptr;
        void* m_queue_compute = nullptr;
        void* m_queue_copy = nullptr;
        uint32_t m_queue_graphics_index = 0;
        uint32_t m_queue_compute_index = 0;
        uint32_t m_queue_copy_index = 0;

        std::unordered_map<uint32_t, RHI_DescriptorSet> m_descriptor_sets;
        void* m_descriptor_pool = nullptr;
        uint32_t m_descriptor_set_capacity = 0;

        std::vector<std::shared_ptr<RHI_CommandPool>> m_cmd_pools;

        uint32_t m_max_texture_1d_dimension = 0;
        uint32_t m_max_texture_2d_dimension = 0;
        uint32_t m_max_texture_3d_dimension = 0;
        uint32_t m_max_texture_cube_dimension = 0;
        uint32_t m_max_texture_array_layers = 0;
        uint64_t m_min_uniform_buffer_offset_alignment = 0;
        float m_timestamp_period = 0;
        bool m_wide_lines = false;
        uint32_t m_max_bound_descriptor_sets = 4;

        // Misc
        uint32_t m_physical_device_index = 0;
        uint32_t m_enabled_graphics_shader_stages = 0;
        mutable std::mutex m_queue_mutex;
        std::vector<PhysicalDevice> m_physical_devices;
        std::shared_ptr<RHI_Context> m_rhi_context;
	};
}