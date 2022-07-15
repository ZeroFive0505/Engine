#pragma once

#include "RHI_Texture.h"
#include "RHI_Viewport.h"

namespace PlayGround
{
    // RHI 텍스쳐 배열
	class RHI_Texture2DArray : public RHI_Texture
	{
    public:
        RHI_Texture2DArray(Context* context, const uint32_t flags = RHI_Texture_Srv, const char* name = nullptr) : RHI_Texture(context)
        {
            m_ResourceType = EResourceType::Texture2dArray;
            m_flags = flags;

            if (name != nullptr)
            {
                m_ObjectName = name;
            }
        }

        RHI_Texture2DArray(Context* context, const uint32_t width, const uint32_t height, const RHI_Format format, const uint32_t array_length, const uint32_t flags, std::string name = "") : RHI_Texture(context)
        {
            m_ObjectName = name;
            m_ResourceType = EResourceType::Texture2dArray;
            m_width = width;
            m_height = height;
            m_viewport = RHI_Viewport(0, 0, static_cast<float>(width), static_cast<float>(height));
            m_format = format;
            m_array_length = array_length;
            m_flags = flags;

            RHI_CreateResource();
        }

        ~RHI_Texture2DArray() = default;
	};
}