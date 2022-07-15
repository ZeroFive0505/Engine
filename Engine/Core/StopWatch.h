#pragma once

#include <chrono>
#include "../EngineDefinition.h"

namespace PlayGround
{
	// �������� Ư�� ���� ����� �ɸ��� �ð��� üũ�ϱ� ���� Ŭ����
	class StopWatch
	{
	public:
		StopWatch()
		{
			Start();
		}

		~StopWatch() = default;

		inline void Start()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		// �ɸ� �ð��� �ʷ� ��ȯ�ؼ� ��ȯ�Ѵ�.
		inline float GetElapsedTimeSec() const
		{
			const std::chrono::duration<double, std::milli> ms = std::chrono::high_resolution_clock::now() - m_Start;
			return static_cast<float>(ms.count() / 1000);
		}

		// �ɸ� �ð��� ms�� ��ȯ�ؼ� ��ȯ�Ѵ�.
		inline float GetElapsedTimeMS() const
		{
			const std::chrono::duration<double, std::milli> ms = std::chrono::high_resolution_clock::now() - m_Start;
			return static_cast<float>(ms.count());
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};
}