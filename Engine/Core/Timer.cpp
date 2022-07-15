#include "Common.h"
#include "Timer.h"
#include "../Display/Display.h"

using namespace std;

namespace PlayGround
{
	// 초기화시 현재 시간을 설정
	Timer::Timer(Context* context) : SubModule(context)
	{
		m_Time_start = chrono::high_resolution_clock::now();
		m_Time_sleep_end = chrono::high_resolution_clock::now();
	}

	void Timer::Update(double delta_time)
	{
		// 현재 시간을 가져온다.
		m_Time_sleep_start = chrono::high_resolution_clock::now();
		// 지나간 시간을 구한다.
		chrono::duration<double, milli> elapsed_time = m_Time_sleep_start - m_Time_sleep_end;

		{
			// 목표로하는 ms초를 구한다.
			double target_ms = 1000.0 / m_FPS_limit;

			// 일부러 공회전을 시켜 프레임 제한을 한다.
			while (elapsed_time.count() < target_ms)
			{
				elapsed_time = chrono::high_resolution_clock::now() - m_Time_sleep_start;
			}

			// 시간 갱신
			m_Time_sleep_end = chrono::high_resolution_clock::now();
		}

		// 델타 타임 계산
		m_DeltaTime_Ms = static_cast<double>(elapsed_time.count());
		// ms 시간을 기록한다.
		m_TimeMs = static_cast<double>(chrono::duration<double, milli>(m_Time_sleep_start - m_Time_start).count());

		// 선형보간한 시간을 계산한다.
		const double frames_to_accmulate = 10;
		const double delta_feedback = 1.0 / frames_to_accmulate;
		m_DeltaTime_smoothed_ms = m_DeltaTime_smoothed_ms * (1.0 - delta_feedback) + m_DeltaTime_Ms * delta_feedback;
	}

	void Timer::SetFPSLimit(double fps)
	{
		if (fps < 0.0)
		{
			const sDisplayMode& display_mode = Display::GetActiveDisplayMode();
			fps = display_mode.hz;
		}


		fps = Math::Util::Clamp(m_FPS_min, m_FPS_max, fps);

		if (m_FPS_limit == fps)
			return;

		// 프레임 제한
		m_User_selected_fps_target = true;
		m_FPS_limit = fps;
		LOG_INFO("Set to %.2f FPS", m_FPS_limit);
	}

	FPSLimitType Timer::GetFPSLimitType()
	{
		if (m_FPS_limit == Display::GetActiveDisplayMode().hz)
			return FPSLimitType::FixedToMonitor;

		if (m_FPS_limit == m_FPS_max)
			return FPSLimitType::Unlocked;

		return FPSLimitType::Fixed;
	}
}