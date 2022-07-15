#pragma once

#include <memory>
#include <array>
#include "RHI_Viewport.h"
#include "RHI_Definition.h"
#include "../Resource/IResource.h"

namespace PlayGround
{
	enum RHI_Texture_Flags : uint32_t
	{
        // Common_buffer.hlsl과 매칭되야함

        RHI_Texture_Srv = 1U << 0,
        RHI_Texture_Uav = 1U << 1,
        RHI_Texture_Rt_Color = 1U << 2,
        RHI_Texture_Rt_DepthStencil = 1U << 3,
        RHI_Texture_Rt_DepthStencilReadOnly = 1U << 4,
        RHI_Texture_PerMipViews = 1U << 5,
        RHI_Texture_Grayscale = 1U << 6,
        RHI_Texture_Transparent = 1U << 7,
        RHI_Texture_Srgb = 1U << 8,
        RHI_Texture_CanBeCleared = 1U << 9,
        RHI_Texture_Mips = 1U << 10,
        RHI_Texture_Compressed = 1U << 11,
        RHI_Texture_Visualise = 1U << 12,
        RHI_Texture_Visualise_Pack = 1U << 13,
        RHI_Texture_Visualise_GammaCorrect = 1U << 14,
        RHI_Texture_Visualise_Boost = 1U << 15,
        RHI_Texture_Visualise_Abs = 1U << 16,
        RHI_Texture_Visualise_Channel_R = 1U << 17,
        RHI_Texture_Visualise_Channel_G = 1U << 18,
        RHI_Texture_Visualise_Channel_B = 1U << 19,
        RHI_Texture_Visualise_Channel_A = 1U << 20,
        RHI_Texture_Visualise_Sample_Point = 1U << 21
	};

    enum RHI_Shader_View_Type : uint8_t
    {
        RHI_Shader_View_ColorDepth,
        RHI_Shader_View_Stencil,
        RHI_Shader_View_Unordered_Access
    };

    struct RHI_Texture_Mip
    {
        std::vector<std::byte> bytes;
    };

    struct RHI_Texture_Slice
    {
        std::vector<RHI_Texture_Mip> mips;
        
        inline uint32_t GetMipCount()
        {
            return static_cast<uint32_t>(mips.size());
        }
    };

    class RHI_Texture : public IResource, public std::enable_shared_from_this<RHI_Texture>
    {
    public:
        RHI_Texture(Context* context);
        ~RHI_Texture();

        bool SaveToFile(const std::string& file_path) override;
        bool LoadFromFile(const std::string& file_path) override;

        inline uint32_t GetWidth()                                const { return m_width; }
        inline void SetWidth(const uint32_t width) { m_width = width; }

        inline uint32_t GetHeight()                               const { return m_height; }
        inline void SetHeight(const uint32_t height) { m_height = height; }

        inline uint32_t GetBitsPerChannel()                       const { return m_bits_per_channel; }
        inline void SetBitsPerChannel(const uint32_t bits) { m_bits_per_channel = bits; }
        inline uint32_t GetBytesPerChannel()                      const { return m_bits_per_channel / 8; }
        inline uint32_t GetBytesPerPixel()                        const { return (m_bits_per_channel / 8) * m_channel_count; }

        inline uint32_t GetChannelCount()                         const { return m_channel_count; }
        inline void SetChannelCount(const uint32_t channel_count) { m_channel_count = channel_count; }

        inline RHI_Format GetFormat()                             const { return m_format; }
        inline void SetFormat(const RHI_Format format) { m_format = format; }

        inline uint32_t GetArrayLength()                          const { return m_array_length; }
        inline uint32_t GetMipCount()                             const { return m_mip_count; }
        inline bool HasData()                                     const { return !m_data.empty() && !m_data[0].mips.empty() && !m_data[0].mips[0].bytes.empty(); };
        std::vector<RHI_Texture_Slice>& GetData() { return m_data; }
        RHI_Texture_Mip& CreateMip(const uint32_t array_index);
        RHI_Texture_Mip& GetMip(const uint32_t array_index, const uint32_t mip_index);
        RHI_Texture_Slice& GetSlice(const uint32_t array_index);

        void SetFlag(const uint32_t flag, bool enabled = true);
        inline uint32_t GetFlags()                 const { return m_flags; }
        inline void SetFlags(const uint32_t flags) { m_flags = flags; }
        inline bool IsSrv()                        const { return m_flags & RHI_Texture_Srv; }
        inline bool IsUav()                        const { return m_flags & RHI_Texture_Uav; }
        inline bool IsRenderTargetDepthStencil()   const { return m_flags & RHI_Texture_Rt_DepthStencil; }
        inline bool IsRenderTargetColor()          const { return m_flags & RHI_Texture_Rt_Color; }
        inline bool HasPerMipViews()               const { return m_flags & RHI_Texture_PerMipViews; }
        inline bool HasMips()                      const { return m_flags & RHI_Texture_Mips; }
        inline bool CanBeCleared()                 const { return m_flags & RHI_Texture_CanBeCleared || IsRenderTargetDepthStencil() || IsRenderTargetColor(); }
        inline bool IsGrayscale()                  const { return m_flags & RHI_Texture_Grayscale; }
        inline bool IsTransparent()                const { return m_flags & RHI_Texture_Transparent; }

        inline bool IsDepthFormat()        const { return m_format == RHI_Format_D16_Unorm || m_format == RHI_Format_D32_Float || m_format == RHI_Format_D32_Float_S8X24_Uint; }
        inline bool IsStencilFormat()      const { return m_format == RHI_Format_D32_Float_S8X24_Uint; }
        inline bool IsDepthStencilFormat() const { return IsDepthFormat() || IsStencilFormat(); }
        inline bool IsColorFormat()        const { return !IsDepthStencilFormat(); }

        void SetLayout(const RHI_Image_Layout layout, RHI_CommandList* cmd_list, const int mip = -1, const bool ranged = true);
        inline RHI_Image_Layout GetLayout(const uint32_t mip) const { return m_layout[mip]; }
        inline std::array<RHI_Image_Layout, 12> GetLayouts()  const { return m_layout; }
        bool DoAllMipsHaveTheSameLayout() const;

        inline const auto& GetViewport() const { return m_viewport; }

        inline void*& GetResource() { return m_resource; }
        inline void* GetResource_View_Srv()                                      const { return m_resource_view_srv; }
        inline void* GetResource_View_Uav()                                      const { return m_resource_view_uav; }
        inline void* GetResource_Views_Srv(const uint32_t i)                     const { return m_resource_views_srv[i]; }
        inline void* GetResource_Views_Uav(const uint32_t i)                     const { return m_resource_views_uav[i]; }
        inline void* GetResource_View_DepthStencil(const uint32_t i = 0)         const { return i < m_resource_view_depthStencil.size() ? m_resource_view_depthStencil[i] : nullptr; }
        inline void* GetResource_View_DepthStencilReadOnly(const uint32_t i = 0) const { return i < m_resource_view_depthStencilReadOnly.size() ? m_resource_view_depthStencilReadOnly[i] : nullptr; }
        inline void* GetResource_View_RenderTarget(const uint32_t i = 0)         const { return i < m_resource_view_renderTarget.size() ? m_resource_view_renderTarget[i] : nullptr; }
        void RHI_DestroyResource(const bool destroy_main, const bool destroy_per_view);

    protected:
        bool Compress(const RHI_Format format);
        bool RHI_CreateResource();
        void RHI_SetLayout(const RHI_Image_Layout new_layout, RHI_CommandList* cmd_list, const int mip_start, const int mip_range);

        uint32_t m_bits_per_channel = 0;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_channel_count = 0;
        uint32_t m_array_length = 1;
        uint32_t m_mip_count = 1;
        RHI_Format m_format = RHI_Format_Undefined;
        uint32_t m_flags = 0;
        std::array<RHI_Image_Layout, 12> m_layout;
        RHI_Viewport m_viewport;
        std::vector<RHI_Texture_Slice> m_data;
        std::shared_ptr<RHI_Device> m_rhi_device;

        void* m_resource = nullptr;
        void* m_resource_view_srv = nullptr;
        void* m_resource_view_uav = nullptr;
        void* m_resource_views_srv[12] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        void* m_resource_views_uav[12] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        std::array<void*, rhi_max_render_target_count> m_resource_view_renderTarget = { nullptr };
        std::array<void*, rhi_max_render_target_count> m_resource_view_depthStencil = { nullptr };
        std::array<void*, rhi_max_render_target_count> m_resource_view_depthStencilReadOnly = { nullptr };

    private:
        void ComputeMemoryUsage();
    };
}