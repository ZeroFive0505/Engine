#include "Common.h"
#include "../RHI_Implementation.h"
#include "../RHI_Pipeline.h"

using namespace std;

namespace PlayGround
{
    RHI_Pipeline::RHI_Pipeline(const RHI_Device* rhi_device, RHI_PipelineState& pipeline_state, RHI_DescriptorSetLayout* descriptor_set_layout)
    {
        m_rhi_device = rhi_device;
        m_state = pipeline_state;
    }

    RHI_Pipeline::~RHI_Pipeline() = default;
}