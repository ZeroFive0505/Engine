#pragma once

#include "../Core/EngineObject.h"
#include <unordered_map>
#include <vector>
#include <array>
#include "RHI_Descriptor.h"

namespace PlayGround
{
    // RHI 디스크립터 셋 레이아웃
	class RHI_DescriptorSetLayout : public EngineObject
	{
    public:
        RHI_DescriptorSetLayout() = default;
        RHI_DescriptorSetLayout(RHI_Device* rhi_device, const std::vector<RHI_Descriptor>& descriptors, const std::string& name);
        ~RHI_DescriptorSetLayout();

        void SetConstantBuffer(const uint32_t slot, RHI_ConstantBuffer* constant_buffer);
        void SetSampler(const uint32_t slot, RHI_Sampler* sampler);
        void SetTexture(const uint32_t slot, RHI_Texture* texture, const int mip, const bool ranged);
        void SetStructuredBuffer(const uint32_t slot, RHI_StructuredBuffer* structured_buffer);

        void ClearDescriptorData();
        RHI_DescriptorSet* GetDescriptorSet();
        const uint32_t* GetDynamicOffsets();
        uint32_t GetConstantBufferCount();
        inline void NeedsToBind() { m_needs_to_bind = true; }
        inline void* GetResource() const { return m_resource; }

    private:
        void CreateResource(const std::vector<RHI_Descriptor>& descriptors);

        void* m_resource = nullptr;
        uint32_t m_hash = 0;

        std::vector<RHI_Descriptor> m_descriptors;

        std::array<uint32_t, PlayGround::rhi_max_constant_buffer_count> m_dynamic_offsets;
        bool m_needs_to_bind = false;
        RHI_Device* m_rhi_device = nullptr;
	};
}