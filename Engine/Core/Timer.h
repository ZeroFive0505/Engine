#pragma once

#include "SubModule.h"
#include <chrono>

namespace PlayGround
{
	class Context;

	// 프레임 제한
	enum class FPSLimitType
	{
		Unlocked, // 무제한
		Fixed, // 고정
		FixedToMonitor // 모니터 주사율에 맞춘다.
	};

	// 델타 타임을 위한 타이머 클래스, 서브 모듈을 상속 받는다.
	class Timer : public SubModule
	{
	public:
		Timer(Context* context);
		~Timer() = default;

		void Update(double delta_time) override;

		void SetFPSLimit(double fps);
		
		// FPS 최대치를 반환한다.
		inline double GetFPSLimit() const
		{
			return m_FPS_limit;
		}

		FPSLimitType GetFPSLimitType();

		// ms를 반환
		inline double GetTimeMS() const
		{
			return m_TimeMs;
		}

		// 초를 반환
		inline float GetTimeSec() const
		{
			return static_cast<float>(m_TimeMs / 1000.0);
		}

		// ms 델타 타임을 반환
		inline double GetDeltaTimeMs() const
		{
			return m_DeltaTime_Ms;
		}

		// 초단위 델타 타임을 반환
		inline float GetDeltaTimeSec() const
		{
			return static_cast<float>(m_DeltaTime_Ms / 1000.0);
		}

		// 선형 보간한 델타 타임 반환
		inline double GetDeltaTimeSmoothedMs() const
		{
			return m_DeltaTime_smoothed_ms;
		}

		// 초단위 델타 타임 반환
		inline float GetDeltaTimeSmoothedSec() const
		{
			return static_cast<float>(m_DeltaTime_smoothed_ms / 1000.0f);
		}

	private:
		std::chrono::high_resolution_clock::time_point m_Time_start;
		std::chrono::high_resolution_clock::time_point m_Time_sleep_start;
		std::chrono::high_resolution_clock::time_point m_Time_sleep_end;
		double m_TimeMs = 0.0;
		double m_DeltaTime_Ms = 0.0;
		double m_DeltaTime_smoothed_ms = 0.0;
		double m_Sleep_overhead = 0.0;

		double m_FPS_min = 10.0;
		double m_FPS_max = 3000.0;
		double m_FPS_limit = m_FPS_min;
		bool m_User_selected_fps_target = false;
	};
}