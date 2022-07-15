#pragma once

#include "RHI_Definition.h"
#include "../Utils/Hash.h"

namespace PlayGround
{
    // RHI 디스크립터
	struct RHI_Descriptor
	{
        RHI_Descriptor() = default;

        RHI_Descriptor(const RHI_Descriptor& descriptor)
        {
            type = descriptor.type;
            layout = descriptor.layout;
            slot = descriptor.slot;
            stage = descriptor.stage;
            name = descriptor.name;
            mip = descriptor.mip;
            array_size = descriptor.array_size;
        }

        RHI_Descriptor(const std::string& name, const RHI_Descriptor_Type type, const RHI_Image_Layout layout, const uint32_t slot, const uint32_t array_size, const uint32_t stage)
        {
            this->type = type;
            this->layout = layout;
            this->slot = slot;
            this->stage = stage;
            this->name = name;
            this->array_size = array_size;
        }

        // 디스크립터 고유 아이디 생성을 위한 해쉬
        uint32_t ComputeHash(bool include_data) const
        {
            uint32_t hash = 0;

            Utility::Hash::HashCombine(hash, slot);
            Utility::Hash::HashCombine(hash, stage);
            Utility::Hash::HashCombine(hash, range);
            Utility::Hash::HashCombine(hash, static_cast<uint32_t>(type));
            Utility::Hash::HashCombine(hash, static_cast<uint32_t>(layout));

            if (include_data)
            {
                Utility::Hash::HashCombine(hash, data);
                Utility::Hash::HashCombine(hash, mip);
            }

            return hash;
        }

        inline bool IsStorage() const { return type == RHI_Descriptor_Type::TextureStorage; }

        uint32_t slot = 0;
        uint32_t stage = 0;
        uint64_t offset = 0;
        uint64_t range = 0;
        uint32_t array_size = 0;
        RHI_Descriptor_Type type = RHI_Descriptor_Type::Undefined;
        RHI_Image_Layout layout = RHI_Image_Layout::Undefined;

        // 데이터
        int mip = -1;
        void* data = nullptr;

        // 이름
        std::string name;
	};
}