#pragma once

#include "SubModule.h"
#include <chrono>

namespace PlayGround
{
	class Context;

	// ������ ����
	enum class FPSLimitType
	{
		Unlocked, // ������
		Fixed, // ����
		FixedToMonitor // ����� �ֻ����� �����.
	};

	// ��Ÿ Ÿ���� ���� Ÿ�̸� Ŭ����, ���� ����� ��� �޴´�.
	class Timer : public SubModule
	{
	public:
		Timer(Context* context);
		~Timer() = default;

		void Update(double delta_time) override;

		void SetFPSLimit(double fps);
		
		// FPS �ִ�ġ�� ��ȯ�Ѵ�.
		inline double GetFPSLimit() const
		{
			return m_FPS_limit;
		}

		FPSLimitType GetFPSLimitType();

		// ms�� ��ȯ
		inline double GetTimeMS() const
		{
			return m_TimeMs;
		}

		// �ʸ� ��ȯ
		inline float GetTimeSec() const
		{
			return static_cast<float>(m_TimeMs / 1000.0);
		}

		// ms ��Ÿ Ÿ���� ��ȯ
		inline double GetDeltaTimeMs() const
		{
			return m_DeltaTime_Ms;
		}

		// �ʴ��� ��Ÿ Ÿ���� ��ȯ
		inline float GetDeltaTimeSec() const
		{
			return static_cast<float>(m_DeltaTime_Ms / 1000.0);
		}

		// ���� ������ ��Ÿ Ÿ�� ��ȯ
		inline double GetDeltaTimeSmoothedMs() const
		{
			return m_DeltaTime_smoothed_ms;
		}

		// �ʴ��� ��Ÿ Ÿ�� ��ȯ
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