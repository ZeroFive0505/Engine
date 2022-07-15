#pragma once
#include "../RHI_Device.h"
#include <vector>
#include <wrl/client.h>
#include <system_error>
#include "../../Log/Logger.h"
#include "../../Display/Display.h"
#include "../RHI_Implementation.h"

namespace PlayGround::d3d11_utility
{
    // ���� ����
	struct globals
	{
		static inline RHI_Device* rhi_device;
		static inline RHI_Context* rhi_context;
	};


    // ���̷�Ʈ X ����
    inline const char* dxgi_error_to_string(const HRESULT error_code)
    {
        switch (error_code)
        {
        case DXGI_ERROR_DEVICE_HUNG:                   return "DXGI_ERROR_DEVICE_HUNG";                   // The application's device failed due to badly formed commands sent by the application. This is an design-time issue that should be investigated and fixed.
        case DXGI_ERROR_DEVICE_REMOVED:                return "DXGI_ERROR_DEVICE_REMOVED";                // The video card has been physically removed from the system, or a driver upgrade for the video card has occurred. The application should destroy and recreate the device. For help debugging the problem, call ID3D10Device::GetDeviceRemovedReason.
        case DXGI_ERROR_DEVICE_RESET:                  return "DXGI_ERROR_DEVICE_RESET";                  // The device failed due to a badly formed command. This is a run-time issue; The application should destroy and recreate the device.
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR:         return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";         // The driver encountered a problem and was put into the device removed state.
        case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:     return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT";     // An event (for example, a power cycle) interrupted the gathering of presentation statistics.
        case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:  return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";  // The application attempted to acquire exclusive ownership of an output, but failed because some other application (or device within the application) already acquired ownership.
        case DXGI_ERROR_INVALID_CALL:                  return "DXGI_ERROR_INVALID_CALL";                  // The application provided invalid parameter data; this must be debugged and fixed before the application is released.
        case DXGI_ERROR_MORE_DATA:                     return "DXGI_ERROR_MORE_DATA";                     // The buffer supplied by the application is not big enough to hold the requested data.
        case DXGI_ERROR_NONEXCLUSIVE:                  return "DXGI_ERROR_NONEXCLUSIVE";                  // A global counter resource is in use, and the Direct3D device can't currently use the counter resource.
        case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:       return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";       // The resource or request is not currently available, but it might become available later.
        case DXGI_ERROR_NOT_FOUND:                     return "DXGI_ERROR_NOT_FOUND";                     // When calling IDXGIObject::GetPrivateData, the GUID passed in is not recognized as one previously passed to IDXGIObject::SetPrivateData or IDXGIObject::SetPrivateDataInterface. When calling IDXGIFentityy::EnumAdapters or IDXGIAdapter::EnumOutputs, the enumerated ordinal is out of range.
        case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:    return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";    // Reserved
        case DXGI_ERROR_REMOTE_OUTOFMEMORY:            return "DXGI_ERROR_REMOTE_OUTOFMEMORY";            // Reserved
        case DXGI_ERROR_WAS_STILL_DRAWING:             return "DXGI_ERROR_WAS_STILL_DRAWING";             // The GPU was busy at the moment when a call was made to perform an operation, and did not execute or schedule the operation.
        case DXGI_ERROR_UNSUPPORTED:                   return "DXGI_ERROR_UNSUPPORTED";                   // The requested functionality is not supported by the device or the driver.
        case DXGI_ERROR_ACCESS_LOST:                   return "DXGI_ERROR_ACCESS_LOST";                   // The desktop duplication interface is invalid. The desktop duplication interface typically becomes invalid when a different type of image is displayed on the desktop.
        case DXGI_ERROR_WAIT_TIMEOUT:                  return "DXGI_ERROR_WAIT_TIMEOUT";                  // The time-out interval elapsed before the next desktop frame was available.
        case DXGI_ERROR_SESSION_DISCONNECTED:          return "DXGI_ERROR_SESSION_DISCONNECTED";          // The Remote Desktop Services session is currently disconnected.
        case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:      return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";      // The DXGI output (monitor) to which the swap chain content was restricted is now disconnected or changed.
        case DXGI_ERROR_CANNOT_PROTECT_CONTENT:        return "DXGI_ERROR_CANNOT_PROTECT_CONTENT";        // DXGI can't provide content protection on the swap chain. This error is typically caused by an older driver, or when you use a swap chain that is incompatible with content protection.
        case DXGI_ERROR_ACCESS_DENIED:                 return "DXGI_ERROR_ACCESS_DENIED";                 // You tried to use a resource to which you did not have the required access privileges. This error is most typically caused when you write to a shared resource with read-only access.
        case DXGI_ERROR_NAME_ALREADY_EXISTS:           return "DXGI_ERROR_NAME_ALREADY_EXISTS";           // The supplied name of a resource in a call to IDXGIResource1::CreateSharedHandle is already associated with some other resource.
        case DXGI_ERROR_SDK_COMPONENT_MISSING:         return "DXGI_ERROR_SDK_COMPONENT_MISSING";         // The application requested an operation that depends on an SDK component that is missing or mismatched.
        case DXGI_ERROR_NOT_CURRENT:                   return "DXGI_ERROR_NOT_CURRENT";                   // The DXGI objects that the application has created are no longer current & need to be recreated for this operation to be performed.
        case DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY:     return "DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY";     // Insufficient HW protected memory exits for proper function.
        case DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION: return "DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION"; // Creating this device would violate the process's dynamic code policy.
        case DXGI_ERROR_NON_COMPOSITED_UI:             return "DXGI_ERROR_NON_COMPOSITED_UI";             // The operation failed because the compositor is not in control of the output.
        case DXGI_DDI_ERR_UNSUPPORTED:                 return "DXGI_DDI_ERR_UNSUPPORTED";
        case E_OUTOFMEMORY:                            return "E_OUTOFMEMORY";
        case E_INVALIDARG:                             return "E_INVALIDARG";                             // One or more arguments are invalid.
        }

        return (std::string("Unknown error code: %d", std::system_category().message(error_code).c_str()).c_str());
    }

    
    constexpr bool error_check(const HRESULT result)
    {
        if (FAILED(result))
        {
            LOG_ERROR("%s", dxgi_error_to_string(result));
            return false;
        }

        return true;
    }

    template <typename T>
    constexpr void release(void*& ptr)
    {
        if (ptr)
        {
            static_cast<T*>(ptr)->Release();
            ptr = nullptr;
        }
    }

    // Ƽ� ����� �����Ǵ��� Ȯ��
    inline bool CheckTearingSupport()
    {
        // ���丮4���� Ȯ���Ŀ� ���丮5�� �Ѿ��
        Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
        HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&factory4));
        BOOL allowTearing = FALSE;

        // ���丮4 ������
        if (SUCCEEDED(result))
        {
            // ���丮5 �õ�
            Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
            result = factory4.As(&factory5);

            // Ƽ�
            if (SUCCEEDED(result))
            {
                result = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
            }
        }

        // ���� ���� Ȯ��
        const bool fullscreen_borderless_support = SUCCEEDED(result) && allowTearing;
        const bool vender_support = !globals::rhi_device->GetPrimaryPhysicalDevice()->IsIntel();

        return fullscreen_borderless_support && vender_support;
    }

    namespace swap_chain
    {
        inline uint32_t validate_flags(uint32_t flags)
        {
            if (flags & RHI_Present_Immediate)
            {
                // Ƽ� Ȯ��
                if (!CheckTearingSupport())
                {
                    flags &= ~RHI_Present_Immediate;
                    LOG_WARNING("Present_Immediate was requested but it's not supported by the adapter.");
                }
            }

            return flags;
        }

        inline UINT get_flags(uint32_t flags)
        {
            UINT d3d11_flags = 0;

            d3d11_flags |= flags & RHI_SwapChain_Allow_Mode_Switch ? DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 0;
            d3d11_flags |= flags & RHI_Present_Immediate ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

            return d3d11_flags;
        }

        inline DXGI_SWAP_EFFECT get_swap_effect(uint32_t flags)
        {
            if (flags & RHI_Swap_Flip_Discard && globals::rhi_device->GetPrimaryPhysicalDevice()->IsIntel())
            {
                LOG_WARNING("Swap_Flip_Discard was requested but it's not supported by Intel adapters, using Swap_Discard instead.");
                flags &= ~RHI_Swap_Flip_Discard;
                flags |= RHI_Swap_Discard;
            }

            if (flags & RHI_Swap_Discard)
                return DXGI_SWAP_EFFECT_DISCARD;

            if (flags & RHI_Swap_Sequential)
                return DXGI_SWAP_EFFECT_SEQUENTIAL;

            if (flags & RHI_Swap_Flip_Discard)
                return DXGI_SWAP_EFFECT_FLIP_DISCARD;

            if (flags & RHI_Swap_Flip_Sequential)
                return DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

            LOG_ERROR("Unable to determine the requested swap effect, opting for DXGI_SWAP_EFFECT_DISCARD");
            return DXGI_SWAP_EFFECT_DISCARD;
        }
    }

    namespace sampler
    {
        inline D3D11_FILTER get_filter(const RHI_Filter filter_min, const RHI_Filter filter_mag, const RHI_Sampler_Mipmap_Mode filter_mipmap, bool anisotropy_enabled, bool comparison_enabled)
        {
            if (anisotropy_enabled)
                return !comparison_enabled ? D3D11_FILTER_ANISOTROPIC : D3D11_FILTER_COMPARISON_ANISOTROPIC;

            if ((filter_min == RHI_Filter::Nearest) && (filter_mag == RHI_Filter::Nearest) && (filter_mipmap == RHI_Sampler_Mipmap_Mode::Nearest)) return !comparison_enabled ? D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            if ((filter_min == RHI_Filter::Nearest) && (filter_mag == RHI_Filter::Nearest) && (filter_mipmap == RHI_Sampler_Mipmap_Mode::Linear))  return !comparison_enabled ? D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR : D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
            if ((filter_min == RHI_Filter::Nearest) && (filter_mag == RHI_Filter::Linear) && (filter_mipmap == RHI_Sampler_Mipmap_Mode::Nearest)) return !comparison_enabled ? D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT : D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
            if ((filter_min == RHI_Filter::Nearest) && (filter_mag == RHI_Filter::Linear) && (filter_mipmap == RHI_Sampler_Mipmap_Mode::Linear))  return !comparison_enabled ? D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR : D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
            if ((filter_min == RHI_Filter::Linear) && (filter_mag == RHI_Filter::Nearest) && (filter_mipmap == RHI_Sampler_Mipmap_Mode::Nearest)) return !comparison_enabled ? D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT : D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
            if ((filter_min == RHI_Filter::Linear) && (filter_mag == RHI_Filter::Nearest) && (filter_mipmap == RHI_Sampler_Mipmap_Mode::Linear))  return !comparison_enabled ? D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR : D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            if ((filter_min == RHI_Filter::Linear) && (filter_mag == RHI_Filter::Linear) && (filter_mipmap == RHI_Sampler_Mipmap_Mode::Nearest)) return !comparison_enabled ? D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT : D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
            if ((filter_min == RHI_Filter::Linear) && (filter_mag == RHI_Filter::Linear) && (filter_mipmap == RHI_Sampler_Mipmap_Mode::Linear))  return !comparison_enabled ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;

            ASSERT(false && "D3D11_Sampler filter not supported.");
            return D3D11_FILTER_MIN_MAG_MIP_POINT;
        }
    }
}