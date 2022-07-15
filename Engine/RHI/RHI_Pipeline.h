#pragma once

#include <memory>
#include "RHI_PipelineState.h"

namespace PlayGround
{
    // RHI 렌더링 파이프 라인
	class RHI_Pipeline : public EngineObject
	{
    public:
        RHI_Pipeline() = default;
        RHI_Pipeline(const RHI_Device* rhi_device, RHI_PipelineState& pipeline_state, RHI_DescriptorSetLayout* descriptor_set_layout);
        ~RHI_Pipeline();

        inline void* GetResource_Pipeline()          const { return m_resource_pipeline; }
        inline void* GetResource_PipelineLayout()    const { return m_resource_pipeline_layout; }
        inline RHI_PipelineState* GetPipelineState() { return &m_state; }

    private:
        RHI_PipelineState m_state;

        void* m_resource_pipeline = nullptr;
        void* m_resource_pipeline_layout = nullptr;

        const RHI_Device* m_rhi_device;
	};
}