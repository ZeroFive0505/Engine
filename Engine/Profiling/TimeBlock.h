#pragma once

#include <chrono>
#include <memory>
#include "../RHI/RHI_Definition.h"


namespace PlayGround
{
	class Renderer;

	// Ÿ�Ӻ���� Ÿ��
	enum class TimeBlockType
	{
		CPU,
		GPU,
		Undefined
	};

	// �������ϸ��� ���� Ÿ�Ӻ��
	class TimeBlock
	{
	public:
		TimeBlock() = default;
		~TimeBlock();

		// �ð� ���� ����
		void Begin(const uint32_t id, const char* name, TimeBlockType type, const TimeBlock* parent = nullptr, RHI_CommandList* cmd_list = nullptr);
		// �ð� ���� ��
		void End();
		// �ɸ� �ð� ���
		void ComputeDuration(const uint32_t pass_index);
		void Reset();

		inline TimeBlockType GetType() const { return m_Type; }

		inline const char* GetName() const { return m_Name; }

		inline const TimeBlock* GetParent() const { return m_Parent; }

		inline uint32_t GetTreeDepth() const { return m_TreeDepth; }

		inline uint32_t GetTreeDepthMax() const { return m_MaxTreeDepth; }

		inline float GetDuration() const { return m_Duration; }

		inline bool IsComplete() const { return m_IsComplete; }

		inline uint32_t GetID() const { return m_ID; }

		void ClearGPUObjects();

	private:
		static uint32_t FindTreeDepth(const TimeBlock* time_block, uint32_t depth = 0);
		static uint32_t m_MaxTreeDepth;

		const char* m_Name = nullptr;
		TimeBlockType m_Type = TimeBlockType::Undefined;
		float m_Duration = 0.0f;
		const TimeBlock* m_Parent = nullptr;
		uint32_t m_TreeDepth = 0;
		bool m_IsComplete = false;
		uint32_t m_ID = 0;

		RHI_Device* m_RhiDevice = nullptr;
		RHI_CommandList* m_CmdList = nullptr;

		std::chrono::steady_clock::time_point m_Start;
		std::chrono::steady_clock::time_point m_End;

		void* m_QueryStart = nullptr;
		void* m_QueryEnd = nullptr;
	};
}