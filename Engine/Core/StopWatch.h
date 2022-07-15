#pragma once

#include <chrono>
#include "../EngineDefinition.h"

namespace PlayGround
{
	// 엔진에서 특정 동작 수행시 걸리는 시간을 체크하기 위한 클래스
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

		// 걸린 시간을 초로 변환해서 반환한다.
		inline float GetElapsedTimeSec() const
		{
			const std::chrono::duration<double, std::milli> ms = std::chrono::high_resolution_clock::now() - m_Start;
			return static_cast<float>(ms.count() / 1000);
		}

		// 걸린 시간을 ms로 변환해서 반환한다.
		inline float GetElapsedTimeMS() const
		{
			const std::chrono::duration<double, std::milli> ms = std::chrono::high_resolution_clock::now() - m_Start;
			return static_cast<float>(ms.count());
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};
}