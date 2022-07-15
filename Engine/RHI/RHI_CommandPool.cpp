#include "Common.h"
#include "RHI_CommandPool.h"

using namespace std;

namespace PlayGround
{
    void RHI_CommandPool::AllocateCommandLists(const uint32_t command_list_count)
    {
        for (uint32_t index_pool = 0; index_pool < static_cast<uint32_t>(m_resources.size()); index_pool++)
        {
            for (uint32_t i = 0; i < command_list_count; i++)
            {
                vector<shared_ptr<RHI_CommandList>>& cmd_lists = m_cmd_lists[index_pool];
                string cmd_list_name = m_ObjectName + "_cmd_pool_" + to_string(index_pool) + "_cmd_list_" + to_string(cmd_lists.size());
                shared_ptr<RHI_CommandList> cmd_list = make_shared<RHI_CommandList>(m_Context, m_resources[index_pool], cmd_list_name.c_str());

                cmd_lists.emplace_back(cmd_list);
            }
        }
    }

    bool RHI_CommandPool::Update()
    {
        if (m_pool_index == -1)
        {
            m_pool_index = 0;
            m_cmd_list_index = 0;

            return false;
        }

        m_cmd_list_index = (m_cmd_list_index + 1) % GetCommandListCount();

        if (m_cmd_list_index == 0)
        {
            Reset();
            return true;
        }

        return false;
    }
}
