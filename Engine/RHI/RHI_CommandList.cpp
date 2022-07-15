#include "Common.h"
#include "RHI_CommandList.h"
#include "RHI_Device.h"
#include "RHI_Fence.h"
#include "RHI_Semaphore.h"
#include "RHI_DescriptorSetLayout.h"
#include "RHI_Shader.h"

using namespace std;

namespace PlayGround
{
    void RHI_CommandList::Wait()
    {
        ASSERT(m_state == RHI_CommandListState::Submitted && "The command list hasn't been submitted, can't wait for it.");

        bool executing = !m_proccessed_fence->IsSignaled() && !m_discard;
        if (executing)
        {
            LOG_WARNING("Waiting for command list \"%s\" to be processed by the queue...", m_ObjectName.c_str());

            if (!m_proccessed_fence->Wait())
            {
                LOG_ERROR("Timed out while waiting for command list \"%s\"", m_ObjectName.c_str());
            }
        }

        m_proccessed_fence->Reset();
        m_state = RHI_CommandListState::Idle;

    }

    void RHI_CommandList::Discard()
    {
        m_discard = true;
    }

    uint32_t RHI_CommandList::Gpu_GetMemory(RHI_Device* rhi_device)
    {
        if (const PhysicalDevice* physical_device = rhi_device->GetPrimaryPhysicalDevice())
        {
            return physical_device->GetMemory();
        }

        return 0;
    }

    bool RHI_CommandList::IsExecuting()
    {
        return m_state == RHI_CommandListState::Submitted && !m_proccessed_fence->IsSignaled();
    }

    void RHI_CommandList::Descriptors_GetDescriptorsFromPipelineState(RHI_PipelineState& pipeline_state, vector<RHI_Descriptor>& descriptors)
    {
        if (!pipeline_state.IsValid())
        {
            LOG_ERROR("Invalid pipeline state");
            descriptors.clear();
            return;
        }

        descriptors.clear();

        bool descriptors_acquired = false;

        if (pipeline_state.IsCompute())
        {
            ASSERT(pipeline_state.shader_compute->GetCompilationState() == Shader_Compilation_State::Succeeded && "Shader hasn't compiled");

            descriptors = pipeline_state.shader_compute->GetDescriptors();
            descriptors_acquired = true;
        }
        else if (pipeline_state.IsGraphics())
        {
            ASSERT(pipeline_state.shader_vertex->GetCompilationState() == Shader_Compilation_State::Succeeded && "Shader hasn't compiled");

            descriptors = pipeline_state.shader_vertex->GetDescriptors();
            descriptors_acquired = true;

            if (pipeline_state.shader_pixel)
            {
                ASSERT(pipeline_state.shader_pixel->GetCompilationState() == Shader_Compilation_State::Succeeded && "Shader hasn't compiled");

                for (const RHI_Descriptor& descriptor_reflected : pipeline_state.shader_pixel->GetDescriptors())
                {
                    bool updated_existing = false;
                    for (RHI_Descriptor& descriptor : descriptors)
                    {
                        bool is_same_resource =
                            (descriptor.type == descriptor_reflected.type) &&
                            (descriptor.slot == descriptor_reflected.slot);

                        if ((descriptor.type == descriptor_reflected.type) && (descriptor.slot == descriptor_reflected.slot))
                        {
                            descriptor.stage |= descriptor_reflected.stage;
                            updated_existing = true;
                            break;
                        }
                    }

                    if (!updated_existing)
                    {
                        descriptors.emplace_back(descriptor_reflected);
                    }
                }
            }
        }
    }
}
