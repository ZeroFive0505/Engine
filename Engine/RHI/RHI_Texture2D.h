#pragma once

#include "RHI_Texture.h"
#include "RHI_Viewport.h"

namespace PlayGround
{
    // RHI ÅØ½ºÃÄ 2D
	class RHI_Texture2D : public RHI_Texture
	{
    public:
        RHI_Texture2D(Context* context, const uint32_t flags = RHI_Texture_Srv, const char* name = nullptr) : RHI_Texture(context)
        {
            m_ResourceType = EResourceType::Texture2d;
            m_flags = flags;

            if (name != nullptr)
            {
                m_ObjectName = name;
            }
        }

        RHI_Texture2D(Context* context, const uint32_t width, const uint32_t height, const RHI_Format format, const uint32_t flags, const std::vector<RHI_Texture_Slice>& data, const char* name = nullptr) : RHI_Texture(context)
        {
            m_ResourceType = EResourceType::Texture2d;
            m_width = width;
            m_height = height;
            m_viewport = RHI_Viewport(0, 0, static_cast<float>(width), static_cast<float>(height));
            m_format = format;
            m_data = data;
            m_mip_count = GetSlice(0).GetMipCount();
            m_flags = flags;
            m_channel_count = RhiFormatToChannelCount(m_format);
            m_bits_per_channel = RhiFormatToBitsPerChannel(m_format);

            if (name != nullptr)
            {
                m_ObjectName = name;
            }

            RHI_Texture2D::RHI_CreateResource();
        }

        RHI_Texture2D(Context* context, const uint32_t width, const uint32_t height, const uint32_t mip_count, const RHI_Format format, const uint32_t flags, const char* name = nullptr) : RHI_Texture(context)
        {
            m_ResourceType = EResourceType::Texture2d;
            m_width = width;
            m_height = height;
            m_viewport = RHI_Viewport(0, 0, static_cast<float>(width), static_cast<float>(height));
            m_format = format;
            m_mip_count = mip_count;
            m_flags = flags;
            m_channel_count = RhiFormatToChannelCount(format);

            if (name != nullptr)
            {
                m_ObjectName = name;
            }

            RHI_Texture2D::RHI_CreateResource();
        }

        ~RHI_Texture2D() = default;
	};
}