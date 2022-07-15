#pragma once

#include "../Core/EngineObject.h"
#include "RHI_Descriptor.h"

namespace PlayGround
{
    // RHI 디스크립터 셋
	class RHI_DescriptorSet : public EngineObject
	{
    public:
        RHI_DescriptorSet() = default;
        ~RHI_DescriptorSet() = default;

        RHI_DescriptorSet(RHI_Device* rhi_device, const std::vector<RHI_Descriptor>& descriptors, RHI_DescriptorSetLayout* descriptor_set_layout, const char* name)
        {
            m_rhi_device = rhi_device;
            if (name) 
                m_ObjectName = name; 
            Create(descriptor_set_layout);
            Update(descriptors);
        }

        inline void* GetResource() { return m_resource; }

    private:
        void Create(RHI_DescriptorSetLayout* descriptor_set_layout);
        void Update(const std::vector<RHI_Descriptor>& descriptors);

        void* m_resource = nullptr;
        RHI_Device* m_rhi_device = nullptr;
	};
}