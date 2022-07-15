#pragma once

#include <memory>
#include <vector>
#include <array>
#include "RHI_Definition.h"
#include "../Core/EngineObject.h"

namespace PlayGround
{
	namespace Math
	{
		class Vector4;
	}

    // RHI 스왑 체인
	class RHI_SwapChain : public EngineObject
	{
    public:
        RHI_SwapChain(
            void* window_handle,
            const std::shared_ptr<RHI_Device>& rhi_device,
            uint32_t width,
            uint32_t height,
            RHI_Format format,
            uint32_t buffer_count,
            uint32_t flags,
            const char* name
        );
        ~RHI_SwapChain();

        bool Resize(uint32_t width, uint32_t height, const bool force = false);
        void Present();

        inline RHI_Image_Layout GetLayout() const { return m_layouts[m_index_image_acquired]; }
        void SetLayout(const RHI_Image_Layout& layout, RHI_CommandList* cmd_list);

        inline uint32_t GetWidth()       const { return m_width; }
        inline uint32_t GetHeight()      const { return m_height; }
        inline uint32_t GetBufferCount() const { return m_buffer_count; }
        inline uint32_t GetFlags()       const { return m_flags; }
        inline uint32_t GetImageIndex()  const { return m_index_image_acquired; }
        inline bool PresentEnabled()     const { return m_present_enabled; }

        inline void* Get_Resource()                   const { return m_backbuffer_resource[0]; }
        inline void* Get_Resource_View()              const { return m_backbuffer_resource_view[m_index_image_acquired]; }
        inline void* Get_Resource_View_RenderTarget() const { return m_resource_view; }

    private:
        void AcquireNextImage();

        bool m_windowed = false;
        uint32_t m_buffer_count = 0;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_flags = 0;
        RHI_Format m_format = RHI_Format_R8G8B8A8_Unorm;
        std::array<RHI_Image_Layout, 4> m_layouts = { RHI_Image_Layout::Undefined, RHI_Image_Layout::Undefined, RHI_Image_Layout::Undefined, RHI_Image_Layout::Undefined };

        void* m_resource = nullptr;
        void* m_resource_view = nullptr;
        void* m_surface = nullptr;
        void* m_window_handle = nullptr;
        bool m_present_enabled = true;
        uint32_t m_semaphore_index = std::numeric_limits<uint32_t>::max();
        RHI_Device* m_rhi_device = nullptr;

        std::array<std::shared_ptr<RHI_Semaphore>, 3> m_semaphore_image_acquired;
        uint32_t m_index_image_acquired = std::numeric_limits<uint32_t>::max();

        std::array<void*, 3> m_backbuffer_resource;
        std::array<void*, 3> m_backbuffer_resource_view;
	};
}