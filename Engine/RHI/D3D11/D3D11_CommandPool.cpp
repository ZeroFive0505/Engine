#include "Common.h"
#include "../RHI_CommandPool.h"
#include "../RHI_Device.h"

using namespace std;

namespace PlayGround
{
    RHI_CommandPool::RHI_CommandPool(RHI_Device* rhi_device, const char* name, const uint64_t swap_chain_id) : EngineObject(rhi_device->GetContext())
    {
        m_rhi_device = rhi_device;
        m_ObjectName = name;
    }

    RHI_CommandPool::~RHI_CommandPool()
    {

    }

    void RHI_CommandPool::Reset()
    {

    }
}