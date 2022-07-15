#pragma once

#include <memory>
#include "../EngineDefinition.h"

/// <summary>
/// ���� �ٽ��� �Ǵ� ����
/// </summary>

namespace PlayGround
{
	// ������ ���ؽ�Ʈ�� Ÿ�̸Ӹ� ������ �ִ´�.
	class Context;
	class Timer;

	// ���� ������ ������Ʈ ����
	enum EEngine_Mode : uint32_t
	{
		PhysicsMode = 1 << 0,
		GameMode = 1 << 1,
		PauseMode = 1 << 2
	};

	class Engine
	{
	public:
		Engine();
		~Engine();

		void Update() const;

		// ���� ���� ��� ��ȯ
		inline uint32_t GetCurrentEngineMode() const
		{
			return m_Flags;
		}

		// ���� ���� ��� ����
		inline void SetCurrentEngineMode(const EEngine_Mode flags)
		{
			m_Flags = flags;
		}

		// ���� ��� ��
		inline void EnableEngineMode(const EEngine_Mode flag)
		{
			m_Flags |= flag;
		}

		// ���� ��� ����
		inline void DisableEngineMode(const EEngine_Mode flag)
		{
			m_Flags &= ~flag;
		}

		// ���� ��� ���
		inline void ToggleEngineMode(const EEngine_Mode flag)
		{
			if (!IsEngineModeSet(flag))
				m_Flags |= flag;
			else
				m_Flags &= ~flag;
		}

		// ��尡 �����Ǿ� �ִ��� Ȯ��
		inline bool IsEngineModeSet(const EEngine_Mode flag) const
		{
			return m_Flags & flag;
		}

		// ���ؽ�Ʈ ��ȯ
		inline auto GetContext() const
		{
			return m_Context.get();
		}

	private:
		uint32_t m_Flags = 0;
		std::shared_ptr<Context> m_Context;
	};
}