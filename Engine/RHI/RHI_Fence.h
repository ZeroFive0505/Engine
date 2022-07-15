#pragma once

#include "../Core/EngineObject.h"
#include "RHI_Definition.h"

namespace PlayGround
{
    // RHI Ææ½º
	class RHI_Fence : public EngineObject
	{
    public:
        RHI_Fence(RHI_Device* rhi_device, const char* name = nullptr);
        ~RHI_Fence();

        bool IsSignaled();

        bool Wait(uint64_t timeout_nanoseconds = 1000000000 /* one second */);

        bool Reset();

        inline void* GetResource() { return m_resource; }

    private:
        void* m_resource = nullptr;
        RHI_Device* m_rhi_device = nullptr;
	};
}