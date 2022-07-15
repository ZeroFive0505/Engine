#include "Common.h"
#include "TimeBlock.h"
#include "../RHI/RHI_Device.h"
#include "../RHI/RHI_CommandList.h"

#include "../Core/Context.h"

using namespace std;

namespace PlayGround
{
	uint32_t TimeBlock::m_MaxTreeDepth = 0;

	TimeBlock::~TimeBlock()
	{
		Reset();
	}

	// 시간 측정 시작
	void TimeBlock::Begin(const uint32_t id, const char* name, TimeBlockType type, const TimeBlock* parent /*= nullptr*/, RHI_CommandList* cmd_list /*= nullptr*/)
	{
		// 아이디
		m_ID = id;
		// 이름
		m_Name = name;
		// 부모
		m_Parent = parent;
		// 최대 깊이
		m_TreeDepth = FindTreeDepth(this);
		// 타입
		m_Type = type;
		// 최대 깊이
		m_MaxTreeDepth = Math::Util::Max(m_MaxTreeDepth, m_TreeDepth);

		// 커맨드 리스트 존재시
		if (cmd_list)
		{
			// 커맨드 리스트와 디바이스 초기화
			m_CmdList = cmd_list;
			m_RhiDevice = cmd_list->GetContext()->GetSubModule<Renderer>()->GetRhiDevice().get();
		}

		// 타임블록의 타입이 CPU일 경우 CPU에서 계산
		if (type == TimeBlockType::CPU)
			m_Start = chrono::high_resolution_clock::now();
		else if (type == TimeBlockType::GPU)
		{
			// GPU일 경우 쿼리를 만들어낸다.
			if (!m_QueryStart)
			{
				m_RhiDevice->QueryCreate(&m_QueryStart, RHI_Query_Type::Timestamp);
				m_RhiDevice->QueryCreate(&m_QueryEnd, RHI_Query_Type::Timestamp);
			}

			// 커맨드리스트에 타임스탬프 시작
			cmd_list->Timestamp_Start(m_QueryStart);
		}
	}

	// 타임 블록 끝
	void TimeBlock::End()
	{
		// CPU일 경우 간단하게 계산
		if (m_Type == TimeBlockType::CPU)
			m_End = chrono::high_resolution_clock::now();
		// GPU일 경우 쿼리를 이용하여 계산한다.
		else if (m_Type == TimeBlockType::GPU)
			m_CmdList->Timestamp_End(m_QueryEnd);

		// 타임블록 끝
		m_IsComplete = true;
	}


	void TimeBlock::ComputeDuration(const uint32_t pass_index)
	{
		// 타임블록이 끝났을 경우에만
		ASSERT(m_IsComplete);

		// 시간차이를 구한다.
		// CPU일 경우는 간단하게 그 차를 구한다.
		if (m_Type == TimeBlockType::CPU)
		{
			const chrono::duration<double, milli> ms = m_End - m_Start;
			m_Duration = static_cast<float>(ms.count());
		}
		// GPU일 경우 커맨드리스트를 이용하여 구한다.
		else if (m_Type == TimeBlockType::GPU)
			m_Duration = m_CmdList->Timestamp_GetDuration(m_QueryStart, m_QueryEnd, pass_index);
	}

	// 타임블록 초기화
	void TimeBlock::Reset()
	{
		m_Name = nullptr;
		m_Parent = nullptr;
		m_TreeDepth = 0;
		m_Duration = 0.0f;
		m_MaxTreeDepth = 0;
		m_Type = TimeBlockType::Undefined;
		m_IsComplete = false;

		if (m_QueryStart != nullptr && m_QueryEnd != nullptr)
		{
			m_RhiDevice->QueryRelease(m_QueryStart);
			m_RhiDevice->QueryRelease(m_QueryEnd);
		}
	}

	void TimeBlock::ClearGPUObjects()
	{
		m_QueryStart = nullptr;
		m_QueryEnd = nullptr;
	}

	uint32_t TimeBlock::FindTreeDepth(const TimeBlock* time_block, uint32_t depth /*= 0*/)
	{
		if (time_block && time_block->GetParent())
			depth = FindTreeDepth(time_block->GetParent(), ++depth);

		return depth;
	}
}