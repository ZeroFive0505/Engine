#include "Common.h"
#include "RHI_DescriptorSetLayout.h"
#include "RHI_ConstantBuffer.h"
#include "RHI_StructuredBuffer.h"
#include "RHI_Sampler.h"
#include "RHI_Texture.h"
#include "RHI_DescriptorSet.h"
#include "RHI_Device.h"

using namespace std;

namespace PlayGround
{
    RHI_DescriptorSetLayout::RHI_DescriptorSetLayout(RHI_Device* rhi_device, const vector<RHI_Descriptor>& descriptors, const string& name)
    {
        m_rhi_device = rhi_device;
        m_descriptors = descriptors;
        m_ObjectName = name;
        m_dynamic_offsets.fill(0);

        CreateResource(m_descriptors);

        for (const RHI_Descriptor& descriptor : m_descriptors)
        {
            Utility::Hash::HashCombine(m_hash, descriptor.ComputeHash(false));
        }
    }

    void RHI_DescriptorSetLayout::SetConstantBuffer(const uint32_t slot, RHI_ConstantBuffer* constant_buffer)
    {
        for (RHI_Descriptor& descriptor : m_descriptors)
        {
            if ((descriptor.type == RHI_Descriptor_Type::ConstantBuffer) && descriptor.slot == slot + rhi_shader_shift_register_b)
            {
                m_needs_to_bind = descriptor.data != constant_buffer ? true : m_needs_to_bind;
                m_needs_to_bind = descriptor.offset != constant_buffer->GetOffset() ? true : m_needs_to_bind;
                m_needs_to_bind = descriptor.range != constant_buffer->GetStride() ? true : m_needs_to_bind;

                descriptor.data = static_cast<void*>(constant_buffer);
                descriptor.offset = constant_buffer->GetOffset();
                descriptor.range = constant_buffer->GetStride();

                return;
            }
        }
    }

    void RHI_DescriptorSetLayout::SetSampler(const uint32_t slot, RHI_Sampler* sampler)
    {
        for (RHI_Descriptor& descriptor : m_descriptors)
        {
            if (descriptor.type == RHI_Descriptor_Type::Sampler && descriptor.slot == slot + rhi_shader_shift_register_s)
            {
                m_needs_to_bind = descriptor.data != sampler ? true : m_needs_to_bind;

                descriptor.data = static_cast<void*>(sampler);

                return;
            }
        }
    }

    void RHI_DescriptorSetLayout::SetTexture(const uint32_t slot, RHI_Texture* texture, const int mip, const bool ranged)
    {
        bool set_individual_mip = mip != -1;
        RHI_Image_Layout layout = texture->GetLayout(set_individual_mip ? mip : 0);
        uint32_t mip_count = ranged ? texture->GetMipCount() : 1;
        ASSERT(layout == RHI_Image_Layout::General || layout == RHI_Image_Layout::Shader_Read_Only_Optimal || layout == RHI_Image_Layout::Depth_Stencil_Read_Only_Optimal);

        ASSERT(texture->IsSrv());

        for (RHI_Descriptor& descriptor : m_descriptors)
        {
            bool is_storage = layout == RHI_Image_Layout::General;
            bool match_type = descriptor.type == (is_storage ? RHI_Descriptor_Type::TextureStorage : RHI_Descriptor_Type::Texture);
            uint32_t shift = is_storage ? rhi_shader_shift_register_u : rhi_shader_shift_register_t;
            bool match_slot = descriptor.slot == (slot + shift);

            if (match_type && match_slot)
            {
                m_needs_to_bind = descriptor.data != texture ? true : m_needs_to_bind;
                m_needs_to_bind = descriptor.layout != layout ? true : m_needs_to_bind;
                m_needs_to_bind = descriptor.mip != mip ? true : m_needs_to_bind;

                descriptor.data = static_cast<void*>(texture);
                descriptor.layout = layout;
                descriptor.mip = mip;
                descriptor.array_size = mip_count;

                return;
            }
        }
    }

    void RHI_DescriptorSetLayout::SetStructuredBuffer(const uint32_t slot, RHI_StructuredBuffer* structured_buffer)
    {
        for (RHI_Descriptor& descriptor : m_descriptors)
        {
            if ((descriptor.type == RHI_Descriptor_Type::StructuredBuffer) && descriptor.slot == slot + rhi_shader_shift_register_u)
            {
                m_needs_to_bind = descriptor.data != structured_buffer ? true : m_needs_to_bind;
                m_needs_to_bind = descriptor.range != structured_buffer->GetObjectSizeGPU() ? true : m_needs_to_bind;

                descriptor.data = static_cast<void*>(structured_buffer);
                descriptor.offset = 0;
                descriptor.range = structured_buffer->GetObjectSizeGPU();

                return;
            }
        }
    }

    void RHI_DescriptorSetLayout::ClearDescriptorData()
    {
        for (RHI_Descriptor& descriptor : m_descriptors)
        {
            descriptor.data = nullptr;
            descriptor.mip = 0;
        }
    }

    RHI_DescriptorSet* RHI_DescriptorSetLayout::GetDescriptorSet()
    {
        RHI_DescriptorSet* descriptor_set = nullptr;

        uint32_t hash = m_hash;
        for (const RHI_Descriptor& descriptor : m_descriptors)
        {
            Utility::Hash::HashCombine(hash, descriptor.data);
            Utility::Hash::HashCombine(hash, descriptor.mip);
        }

        unordered_map<uint32_t, RHI_DescriptorSet>& descriptor_sets = m_rhi_device->GetDescriptorSets();
        const auto it = descriptor_sets.find(hash);
        if (it == descriptor_sets.end())
        {
            ASSERT(m_rhi_device->HasDescriptorSetCapacity() && "Descriptor pool has no more memory to allocate another descriptor set");

            descriptor_sets[hash] = RHI_DescriptorSet(m_rhi_device, m_descriptors, this, m_ObjectName.c_str());

            descriptor_set = &descriptor_sets[hash];
        }
        else if (m_needs_to_bind) 
        {
            descriptor_set = &it->second;
            m_needs_to_bind = false;
        }

        return descriptor_set;
    }

    const uint32_t* RHI_DescriptorSetLayout::GetDynamicOffsets()
    {
        uint32_t i = 0;
        for (RHI_Descriptor& descriptor : m_descriptors)
        {
            if (descriptor.type == RHI_Descriptor_Type::ConstantBuffer)
            {
                m_dynamic_offsets[i++] = static_cast<uint32_t>(descriptor.offset);
            }
        }

        return m_dynamic_offsets.data();
    }

    uint32_t RHI_DescriptorSetLayout::GetConstantBufferCount()
    {
        uint32_t constant_buffer_count = 0;

        for (RHI_Descriptor& descriptor : m_descriptors)
        {
            if (descriptor.type == RHI_Descriptor_Type::ConstantBuffer)
            {
                constant_buffer_count++;
            }
        }

        return constant_buffer_count;
    }
}