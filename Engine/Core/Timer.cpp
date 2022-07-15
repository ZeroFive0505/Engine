#include "Common.h"
#include "Timer.h"
#include "../Display/Display.h"

using namespace std;

namespace PlayGround
{
	// �ʱ�ȭ�� ���� �ð��� ����
	Timer::Timer(Context* context) : SubModule(context)
	{
		m_Time_start = chrono::high_resolution_clock::now();
		m_Time_sleep_end = chrono::high_resolution_clock::now();
	}

	void Timer::Update(double delta_time)
	{
		// ���� �ð��� �����´�.
		m_Time_sleep_start = chrono::high_resolution_clock::now();
		// ������ �ð��� ���Ѵ�.
		chrono::duration<double, milli> elapsed_time = m_Time_sleep_start - m_Time_sleep_end;

		{
			// ��ǥ���ϴ� ms�ʸ� ���Ѵ�.
			double target_ms = 1000.0 / m_FPS_limit;

			// �Ϻη� ��ȸ���� ���� ������ ������ �Ѵ�.
			while (elapsed_time.count() < target_ms)
			{
				elapsed_time = chrono::high_resolution_clock::now() - m_Time_sleep_start;
			}

			// �ð� ����
			m_Time_sleep_end = chrono::high_resolution_clock::now();
		}

		// ��Ÿ Ÿ�� ���
		m_DeltaTime_Ms = static_cast<double>(elapsed_time.count());
		// ms �ð��� ����Ѵ�.
		m_TimeMs = static_cast<double>(chrono::duration<double, milli>(m_Time_sleep_start - m_Time_start).count());

		// ���������� �ð��� ����Ѵ�.
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

		// ������ ����
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