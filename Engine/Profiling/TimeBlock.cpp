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

	// �ð� ���� ����
	void TimeBlock::Begin(const uint32_t id, const char* name, TimeBlockType type, const TimeBlock* parent /*= nullptr*/, RHI_CommandList* cmd_list /*= nullptr*/)
	{
		// ���̵�
		m_ID = id;
		// �̸�
		m_Name = name;
		// �θ�
		m_Parent = parent;
		// �ִ� ����
		m_TreeDepth = FindTreeDepth(this);
		// Ÿ��
		m_Type = type;
		// �ִ� ����
		m_MaxTreeDepth = Math::Util::Max(m_MaxTreeDepth, m_TreeDepth);

		// Ŀ�ǵ� ����Ʈ �����
		if (cmd_list)
		{
			// Ŀ�ǵ� ����Ʈ�� ����̽� �ʱ�ȭ
			m_CmdList = cmd_list;
			m_RhiDevice = cmd_list->GetContext()->GetSubModule<Renderer>()->GetRhiDevice().get();
		}

		// Ÿ�Ӻ���� Ÿ���� CPU�� ��� CPU���� ���
		if (type == TimeBlockType::CPU)
			m_Start = chrono::high_resolution_clock::now();
		else if (type == TimeBlockType::GPU)
		{
			// GPU�� ��� ������ ������.
			if (!m_QueryStart)
			{
				m_RhiDevice->QueryCreate(&m_QueryStart, RHI_Query_Type::Timestamp);
				m_RhiDevice->QueryCreate(&m_QueryEnd, RHI_Query_Type::Timestamp);
			}

			// Ŀ�ǵ帮��Ʈ�� Ÿ�ӽ����� ����
			cmd_list->Timestamp_Start(m_QueryStart);
		}
	}

	// Ÿ�� ��� ��
	void TimeBlock::End()
	{
		// CPU�� ��� �����ϰ� ���
		if (m_Type == TimeBlockType::CPU)
			m_End = chrono::high_resolution_clock::now();
		// GPU�� ��� ������ �̿��Ͽ� ����Ѵ�.
		else if (m_Type == TimeBlockType::GPU)
			m_CmdList->Timestamp_End(m_QueryEnd);

		// Ÿ�Ӻ�� ��
		m_IsComplete = true;
	}


	void TimeBlock::ComputeDuration(const uint32_t pass_index)
	{
		// Ÿ�Ӻ���� ������ ��쿡��
		ASSERT(m_IsComplete);

		// �ð����̸� ���Ѵ�.
		// CPU�� ���� �����ϰ� �� ���� ���Ѵ�.
		if (m_Type == TimeBlockType::CPU)
		{
			const chrono::duration<double, milli> ms = m_End - m_Start;
			m_Duration = static_cast<float>(ms.count());
		}
		// GPU�� ��� Ŀ�ǵ帮��Ʈ�� �̿��Ͽ� ���Ѵ�.
		else if (m_Type == TimeBlockType::GPU)
			m_Duration = m_CmdList->Timestamp_GetDuration(m_QueryStart, m_QueryEnd, pass_index);
	}

	// Ÿ�Ӻ�� �ʱ�ȭ
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