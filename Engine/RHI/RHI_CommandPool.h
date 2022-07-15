#pragma once

#include "../Core/EngineObject.h"
#include "RHI_Definition.h"
#include "RHI_CommandList.h"
#include <vector>

namespace PlayGround
{
    // 커맨드 리스트를 담는 커맨드 풀
	class RHI_CommandPool : public EngineObject
	{
    public:
        RHI_CommandPool(RHI_Device* rhi_device, const char* name, const uint64_t swap_chain_id);
        ~RHI_CommandPool();

        void AllocateCommandLists(const uint32_t command_list_count);
        bool Update();

        inline RHI_CommandList* GetCurrentCommandList() { return m_cmd_lists[m_pool_index][m_cmd_list_index].get(); }
        inline uint32_t GetCommandListCount()           const { return static_cast<uint32_t>(m_cmd_lists[0].size()); }
        inline uint32_t GetCommandListIndex()           const { return m_cmd_list_index; }
        inline void*& GetResource() { return m_resources[m_pool_index]; }
        inline uint64_t GetSwapchainId()                const { return m_swap_chain_id; }

    private:
        void Reset();

        std::array<std::vector<std::shared_ptr<RHI_CommandList>>, 2> m_cmd_lists;
        int m_cmd_list_index = -1;

        std::array<void*, 2> m_resources;
        int m_pool_index = -1;

        uint64_t m_swap_chain_id = 0;

        RHI_Device* m_rhi_device = nullptr;
	};
}
