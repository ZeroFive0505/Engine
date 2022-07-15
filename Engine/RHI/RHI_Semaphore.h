#pragma once

#include "../Core/EngineObject.h"
#include "RHI_Definition.h"

namespace PlayGround
{
	class RHI_Semaphore : public EngineObject
	{
    public:
        RHI_Semaphore(RHI_Device* rhi_device, bool is_timeline = false, const char* name = nullptr);
        ~RHI_Semaphore();

        void Reset();

        inline bool IsTimelineSemaphore() const { return m_is_timeline; }
        bool Wait(const uint64_t value, const uint64_t timeout = std::numeric_limits<uint64_t>::max());
        bool Signal(const uint64_t value);
        uint64_t GetValue();

        inline RHI_Semaphore_State GetState()                  const { return m_state; }
        inline void SetState(const RHI_Semaphore_State state) { m_state = state; }
        inline void* GetResource() { return m_resource; }

    private:
        void* m_resource = nullptr;
        bool m_is_timeline = false;
        RHI_Semaphore_State m_state = RHI_Semaphore_State::Idle;

        RHI_Device* m_rhi_device = nullptr;
	};
}
