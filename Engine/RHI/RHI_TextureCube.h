#pragma once

#include "RHI_Texture.h"

namespace PlayGround
{
    // RHI 큐브맵 텍스쳐
	class RHI_TextureCube : public RHI_Texture
	{
    public:
        RHI_TextureCube(Context* context) : RHI_Texture(context) { m_ResourceType = EResourceType::TextureCube; }

        RHI_TextureCube(Context* context, const uint32_t width, const uint32_t height, const RHI_Format format, const uint32_t flags, const std::vector<RHI_Texture_Slice>& data) : RHI_Texture(context)
        {
            m_ResourceType = EResourceType::TextureCube;
            m_width = width;
            m_height = height;
            m_viewport = RHI_Viewport(0, 0, static_cast<float>(width), static_cast<float>(height));
            m_format = format;
            m_data = data;
            m_array_length = 6;
            m_mip_count = GetSlice(0).GetMipCount();
            m_flags = flags;
            m_channel_count = RhiFormatToChannelCount(m_format);
            m_bits_per_channel = RhiFormatToBitsPerChannel(m_format);

            RHI_TextureCube::RHI_CreateResource();
        }

        RHI_TextureCube(Context* context, const uint32_t width, const uint32_t height, const RHI_Format format, const uint32_t flags, std::string name = "") : RHI_Texture(context)
        {
            m_ObjectName = name;
            m_ResourceType = EResourceType::TextureCube;
            m_width = width;
            m_height = height;
            m_viewport = RHI_Viewport(0, 0, static_cast<float>(width), static_cast<float>(height));
            m_format = format;
            m_array_length = 6;
            m_flags = flags;

            RHI_TextureCube::RHI_CreateResource();
        }

        ~RHI_TextureCube() = default;
	};
}