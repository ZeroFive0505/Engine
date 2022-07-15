#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_Texture2D.h"
#include "../RHI_Texture2DArray.h"
#include "../RHI_TextureCube.h"
#include "../RHI_CommandList.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
    // 텍스쳐 바인드 플래그 반환
    static UINT get_bind_flags(uint32_t flags)
    {
        UINT flags_d3d11 = 0;

        flags_d3d11 |= (flags & RHI_Texture_Srv) ? D3D11_BIND_SHADER_RESOURCE : 0;
        flags_d3d11 |= (flags & RHI_Texture_Uav) ? D3D11_BIND_UNORDERED_ACCESS : 0;
        flags_d3d11 |= (flags & RHI_Texture_Rt_DepthStencil) ? D3D11_BIND_DEPTH_STENCIL : 0;
        flags_d3d11 |= (flags & RHI_Texture_Rt_Color) ? D3D11_BIND_RENDER_TARGET : 0;

        return flags_d3d11;
    }

    // 깊이 포맷 반환
    static DXGI_FORMAT get_depth_format(RHI_Format format)
    {
        if (format == RHI_Format_D32_Float_S8X24_Uint)
            return DXGI_FORMAT_R32G8X24_TYPELESS;

        if (format == RHI_Format_D32_Float)
            return DXGI_FORMAT_R32_TYPELESS;

        return d3d11_format[format];
    }

    // 깊이 - 스텐실 뷰의 포맷을 가져온다.
    static DXGI_FORMAT get_depth_format_dsv(RHI_Format format)
    {
        if (format == RHI_Format_D32_Float_S8X24_Uint)
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

        if (format == RHI_Format_D32_Float)
            return DXGI_FORMAT_D32_FLOAT;

        return d3d11_format[format];
    }

    // 쉐이더 리소스 뷰의 포맷을 가져온다.
    static DXGI_FORMAT get_depth_format_srv(RHI_Format format)
    {
        if (format == RHI_Format_D32_Float_S8X24_Uint)
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        else if (format == RHI_Format_D32_Float)
            return DXGI_FORMAT_R32_FLOAT;

        return d3d11_format[format];
    }

    // 텍스쳐 생성
    static bool create_texture(
        ID3D11Texture2D*& texture,
        const EResourceType resource_type,
        const uint32_t width,
        const uint32_t height,
        const uint32_t channel_count,
        const uint32_t array_size,
        const uint32_t mip_count,
        const uint32_t bits_per_channel,
        const DXGI_FORMAT format,
        const UINT flags,
        vector<RHI_Texture_Slice>& data,
        const shared_ptr<RHI_Device>& rhi_device
    )
    {
        ASSERT(width != 0);
        ASSERT(height != 0);
        ASSERT(array_size != 0);
        ASSERT(mip_count != 0);

        const bool has_data = !data.empty() && !data[0].mips.empty() && !data[0].mips[0].bytes.empty();

        // 텍스쳐 디스크립션
        D3D11_TEXTURE2D_DESC texture_desc = {};
        texture_desc.Width = static_cast<UINT>(width);
        texture_desc.Height = static_cast<UINT>(height);
        texture_desc.ArraySize = static_cast<UINT>(array_size);
        texture_desc.MipLevels = static_cast<UINT>(mip_count);
        texture_desc.Format = format;
        texture_desc.SampleDesc.Count = 1;
        texture_desc.SampleDesc.Quality = 0;
        texture_desc.Usage = (has_data && !(flags & D3D11_BIND_UNORDERED_ACCESS)) ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
        texture_desc.BindFlags = flags;
        texture_desc.MiscFlags = 0;
        texture_desc.CPUAccessFlags = 0;

        // 큐브 텍스쳐
        if (resource_type == EResourceType::TextureCube)
        {
            // 렌더 타겟 용인지
            bool is_rt = flags & D3D11_BIND_RENDER_TARGET;
            // 깊이 - 스텐실 용인지
            bool is_depth_stencil = flags & D3D11_BIND_DEPTH_STENCIL;

            texture_desc.Usage = (is_rt || is_depth_stencil || !has_data) ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
            texture_desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
        }

        // 초기 데이터
        vector<D3D11_SUBRESOURCE_DATA> texture_data;
        if (has_data)
        {
            ASSERT(channel_count != 0);
            ASSERT(bits_per_channel != 0);

            
            for (uint32_t index_array = 0; index_array < array_size; index_array++)
            {
                for (uint32_t index_mip = 0; index_mip < mip_count; index_mip++)
                {
                    D3D11_SUBRESOURCE_DATA& subresource_data = texture_data.emplace_back(D3D11_SUBRESOURCE_DATA{});
                    // 데이터 포인터
                    subresource_data.pSysMem = data[index_array].mips[index_mip].bytes.data();                
                    // 한 라인의 바이트 크기
                    subresource_data.SysMemPitch = (width >> index_mip) * channel_count * (bits_per_channel / 8);
                    // 3D 텍스쳐 전용
                    subresource_data.SysMemSlicePitch = 0;                                                             
                }
            }
        }

        // 생성
        return d3d11_utility::error_check(rhi_device->GetContextRhi()->device->CreateTexture2D(&texture_desc, texture_data.data(), &texture));
    }

    // 렌더 타겟 뷰 생성
    static bool create_render_target_view(void* texture, array<void*, rhi_max_render_target_count>& views, const EResourceType resource_type, const DXGI_FORMAT format, const uint32_t array_size, const shared_ptr<RHI_Device>& rhi_device)
    {
        ASSERT(texture != nullptr);

        // 디스크립션
        D3D11_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format = format;
        desc.ViewDimension = resource_type == EResourceType::Texture2d ? D3D11_RTV_DIMENSION_TEXTURE2D : D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.MipSlice = 0;
        desc.Texture2DArray.ArraySize = 1;

        // 생성
        for (uint32_t i = 0; i < array_size; i++)
        {
            desc.Texture2DArray.FirstArraySlice = i;

            if (!d3d11_utility::error_check(rhi_device->GetContextRhi()->device->CreateRenderTargetView(static_cast<ID3D11Resource*>(texture), &desc, reinterpret_cast<ID3D11RenderTargetView**>(&views[i]))))
                return false;
        }

        return true;
    }

    // 깊이 - 스텐실 뷰 생성
    static bool create_depth_stencil_view(void* texture, array<void*, rhi_max_render_target_count>& views, const EResourceType resource_type, const DXGI_FORMAT format, const uint32_t array_size, const bool has_stencil, const bool read_only, const shared_ptr<RHI_Device>& rhi_device)
    {
        ASSERT(texture != nullptr);

        // 디스크립션
        D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format = format;
        desc.ViewDimension = resource_type == EResourceType::Texture2d ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.MipSlice = 0;
        desc.Texture2DArray.ArraySize = 1;
        desc.Flags = 0;

        if (read_only)
        {
            desc.Flags |= D3D11_DSV_READ_ONLY_DEPTH;

            if (has_stencil)
            {
                desc.Flags |= D3D11_DSV_READ_ONLY_STENCIL;
            }
        }

        // 생성
        for (uint32_t i = 0; i < array_size; i++)
        {
            desc.Texture2DArray.FirstArraySlice = i;

            if (!d3d11_utility::error_check(rhi_device->GetContextRhi()->device->CreateDepthStencilView(static_cast<ID3D11Resource*>(texture), &desc, reinterpret_cast<ID3D11DepthStencilView**>(&views[i]))))
                return false;
        }

        return true;
    }

    // 쉐이더 리소스 뷰 생성
    static bool create_shader_resource_view(void* texture, void*& view, const EResourceType resource_type, const DXGI_FORMAT format, const uint32_t array_size, const uint32_t mip_count, const uint32_t top_mip, const shared_ptr<RHI_Device>& rhi_device)
    {
        ASSERT(texture != nullptr);

        // 디스크립션
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = format;

        // 일반 텍스쳐의 경우
        if (resource_type == EResourceType::Texture2d)
        {
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2DArray.FirstArraySlice = 0;
            desc.Texture2DArray.MostDetailedMip = top_mip;
            desc.Texture2DArray.MipLevels = static_cast<UINT>(mip_count);
            desc.Texture2DArray.ArraySize = static_cast<UINT>(array_size);
        }
        // 텍스쳐 어레이의 경우
        else if (resource_type == EResourceType::Texture2dArray)
        {
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.FirstArraySlice = 0;
            desc.Texture2DArray.MostDetailedMip = 0;
            desc.Texture2DArray.MipLevels = static_cast<UINT>(mip_count);
            desc.Texture2DArray.ArraySize = static_cast<UINT>(array_size);
        }
        // 큐브 텍스쳐의 경우
        else if (resource_type == EResourceType::TextureCube)
        {
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            desc.TextureCube.MipLevels = static_cast<UINT>(mip_count);
            desc.TextureCube.MostDetailedMip = 0;
        }

        // 생성
        return d3d11_utility::error_check(rhi_device->GetContextRhi()->device->CreateShaderResourceView(static_cast<ID3D11Resource*>(texture), &desc, reinterpret_cast<ID3D11ShaderResourceView**>(&view)));
    }

    // UAV 생성
    static bool create_unordered_access_view(void* texture, void*& view, const EResourceType resource_type, const DXGI_FORMAT format, const uint32_t array_size, const uint32_t mip, const shared_ptr<RHI_Device>& rhi_device)
    {
        ASSERT(texture != nullptr);

        // 디스크립션
        D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.Format = format;

        // 일반 텍스처일 경우
        if (resource_type == EResourceType::Texture2d)
        {
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipSlice = mip;
        }
        // 텍스쳐 어레이일 경우
        else if (resource_type == EResourceType::Texture2dArray)
        {
            desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.MipSlice = mip;
            desc.Texture2DArray.FirstArraySlice = 0;
            desc.Texture2DArray.ArraySize = static_cast<UINT>(array_size);
        }

        // 생성
        return d3d11_utility::error_check(rhi_device->GetContextRhi()->device->CreateUnorderedAccessView(static_cast<ID3D11Resource*>(texture), &desc, reinterpret_cast<ID3D11UnorderedAccessView**>(&view)));
    }

    void RHI_Texture::RHI_SetLayout(const RHI_Image_Layout new_layout, RHI_CommandList* cmd_list, const int mip_start, const int mip_range)
    {

    }

    bool RHI_Texture::RHI_CreateResource()
    {
        // Validate
        ASSERT(m_rhi_device != nullptr);
        ASSERT(m_rhi_device->GetContextRhi()->device != nullptr);

        bool result_tex = true;
        bool result_srv = true;
        bool result_uav = true;
        bool result_rt = true;
        bool result_ds = true;

        // Get texture flags
        const UINT flags = get_bind_flags(m_flags);

        // Resolve formats
        const DXGI_FORMAT format = get_depth_format(m_format);
        const DXGI_FORMAT format_dsv = get_depth_format_dsv(m_format);
        const DXGI_FORMAT format_srv = get_depth_format_srv(m_format);

        ID3D11Texture2D* resource = nullptr;

        // TEXTURE
        result_tex = create_texture
        (
            resource,
            m_ResourceType,
            m_width,
            m_height,
            m_channel_count,
            m_array_length,
            m_mip_count,
            m_bits_per_channel,
            format,
            flags,
            m_data,
            m_rhi_device
        );

        ASSERT(resource != nullptr);

        // RESOURCE VIEW
        if (IsSrv())
        {
            result_srv = create_shader_resource_view(
                resource,
                m_resource_view_srv,
                m_ResourceType,
                format_srv,
                m_array_length,
                m_mip_count,
                0,
                m_rhi_device
            );

            if (HasPerMipViews())
            {
                for (uint32_t i = 0; i < m_mip_count; i++)
                {
                    bool result = create_shader_resource_view(
                        resource,
                        m_resource_views_srv[i],
                        m_ResourceType,
                        format_srv,
                        m_array_length,
                        1,
                        i,
                        m_rhi_device
                    );

                    result_srv = !result ? false : result_srv;
                }
            }
        }

        // UNORDERED ACCESS VIEW
        if (IsUav())
        {
            result_uav = create_unordered_access_view(
                resource,
                m_resource_view_uav,
                m_ResourceType,
                format,
                m_array_length,
                0,
                m_rhi_device
            );

            if (HasPerMipViews())
            {
                for (uint32_t i = 0; i < m_mip_count; i++)
                {
                    bool result = create_unordered_access_view(
                        resource,
                        m_resource_views_uav[i],
                        m_ResourceType,
                        format,
                        1,
                        i,
                        m_rhi_device
                    );

                    result_uav = !result ? false : result_uav;
                }
            }
        }

        // DEPTH-STENCIL VIEW
        if (IsRenderTargetDepthStencil())
        {
            result_ds = create_depth_stencil_view
            (
                resource,
                m_resource_view_depthStencil,
                m_ResourceType,
                format_dsv,
                m_array_length,
                IsStencilFormat(),
                false,
                m_rhi_device
            );

            if (m_flags & RHI_Texture_Rt_DepthStencilReadOnly)
            {
                result_ds = create_depth_stencil_view
                (
                    resource,
                    m_resource_view_depthStencilReadOnly,
                    m_ResourceType,
                    format_dsv,
                    m_array_length,
                    IsStencilFormat(),
                    true,
                    m_rhi_device
                );
            }
        }

        // RENDER TARGET VIEW
        if (IsRenderTargetColor())
        {
            result_rt = create_render_target_view
            (
                resource,
                m_resource_view_renderTarget,
                m_ResourceType,
                format,
                m_array_length,
                m_rhi_device
            );
        }

        m_resource = static_cast<void*>(resource);

        return result_tex && result_srv && result_uav && result_rt && result_ds;
    }

    void RHI_Texture::RHI_DestroyResource(const bool destroy_main, const bool destroy_per_view)
    {
        if (destroy_main)
        {
            d3d11_utility::release<ID3D11Texture2D>(m_resource);
            d3d11_utility::release<ID3D11ShaderResourceView>(m_resource_view_srv);
            d3d11_utility::release<ID3D11UnorderedAccessView>(m_resource_view_uav);

            for (void*& resource : m_resource_view_renderTarget)
            {
                d3d11_utility::release<ID3D11RenderTargetView>(resource);
            }

            for (void*& resource : m_resource_view_depthStencil)
            {
                d3d11_utility::release<ID3D11DepthStencilView>(resource);
            }

            for (void*& resource : m_resource_view_depthStencilReadOnly)
            {
                d3d11_utility::release<ID3D11DepthStencilView>(resource);
            }
        }

        if (destroy_per_view)
        {
            for (uint32_t i = 0; i < m_mip_count; i++)
            {
                d3d11_utility::release<ID3D11ShaderResourceView>(m_resource_views_srv[i]);
                d3d11_utility::release<ID3D11UnorderedAccessView>(m_resource_views_uav[i]);
            }
        }
    }
}