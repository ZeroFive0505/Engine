#include "Common.h"
#include "../RHI_Semaphore.h"

using namespace std;

namespace PlayGround
{
    RHI_Semaphore::RHI_Semaphore(RHI_Device* rhi_device, bool is_timeline /*= false*/, const char* name /*= nullptr*/)
    {
        m_is_timeline = is_timeline;
        m_rhi_device = rhi_device;
    }

    RHI_Semaphore::~RHI_Semaphore()
    {

    }

    void RHI_Semaphore::Reset()
    {

    }
}