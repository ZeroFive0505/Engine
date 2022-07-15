#include "Common.h"
#include "../RHI_Fence.h"
#include "../RHI_Implementation.h"
#include "../RHI_Device.h"

namespace PlayGround
{
    RHI_Fence::RHI_Fence(RHI_Device* rhi_device, const char* name /*= nullptr*/)
    {

    }

    RHI_Fence::~RHI_Fence()
    {

    }

    bool RHI_Fence::IsSignaled()
    {
        return true;
    }

    bool RHI_Fence::Wait(uint64_t timeout /*= std::numeric_limits<uint64_t>::max()*/)
    {
        return true;
    }

    bool RHI_Fence::Reset()
    {
        return true;
    }
}