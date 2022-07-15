#pragma once

#include <memory>
#include "../EngineDefinition.h"

/// <summary>
/// 가장 핵심이 되는 엔진
/// </summary>

namespace PlayGround
{
	// 엔진은 컨텍스트와 타이머를 가지고 있는다.
	class Context;
	class Timer;

	// 현재 엔진의 업데이트 상태
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

		// 현재 엔진 모드 반환
		inline uint32_t GetCurrentEngineMode() const
		{
			return m_Flags;
		}

		// 현재 엔진 모드 설정
		inline void SetCurrentEngineMode(const EEngine_Mode flags)
		{
			m_Flags = flags;
		}

		// 엔진 모드 온
		inline void EnableEngineMode(const EEngine_Mode flag)
		{
			m_Flags |= flag;
		}

		// 엔진 모드 오프
		inline void DisableEngineMode(const EEngine_Mode flag)
		{
			m_Flags &= ~flag;
		}

		// 엔진 모드 토글
		inline void ToggleEngineMode(const EEngine_Mode flag)
		{
			if (!IsEngineModeSet(flag))
				m_Flags |= flag;
			else
				m_Flags &= ~flag;
		}

		// 모드가 설정되어 있는지 확인
		inline bool IsEngineModeSet(const EEngine_Mode flag) const
		{
			return m_Flags & flag;
		}

		// 컨텍스트 반환
		inline auto GetContext() const
		{
			return m_Context.get();
		}

	private:
		uint32_t m_Flags = 0;
		std::shared_ptr<Context> m_Context;
	};
}