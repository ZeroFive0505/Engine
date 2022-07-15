#include "Common.h"
#include "Display.h"
#include <Windows.h>
#include "../Core/Timer.h"
#include <vector>
#include "../Core/Context.h"

#pragma comment (lib, "User32")

namespace PlayGround
{
	std::vector<sDisplayMode> Display::m_vecDisplayModes;
	sDisplayMode Display::m_CurrentDisplayMode;

	void Display::RegisterDisplayMode(const sDisplayMode& display_mode, const bool update_fps_limit_to_highest_hz, Context* context)
	{
		// 만약 이미 저장되어있는 디스플레이 모드라면 반환
		for (const sDisplayMode& display : m_vecDisplayModes)
		{
			if (display_mode == display)
				return;
		}

		m_vecDisplayModes.emplace_back(display_mode);

		// 주파수를 내림차순으로 정렬한다.
		sort(m_vecDisplayModes.begin(), m_vecDisplayModes.end(), [](const sDisplayMode& a, const sDisplayMode& b)
		{
			return a.hz > b.hz;
		});


		for (const sDisplayMode& display_mode : m_vecDisplayModes)
		{
			// 만약 높이 넓이가 더 크다면
			if (display_mode.width > m_CurrentDisplayMode.width || display_mode.height > m_CurrentDisplayMode.height)
			{
				// 주파수도 더 높다면
				if (display_mode.hz >= m_CurrentDisplayMode.hz)
				{
					// 현재 디스플레이 모드를 바꾼다.
					m_CurrentDisplayMode.width = display_mode.width;
					m_CurrentDisplayMode.height = display_mode.height;
					m_CurrentDisplayMode.hz = display_mode.hz;
					m_CurrentDisplayMode.numerator = display_mode.numerator;
					m_CurrentDisplayMode.denominator = display_mode.denominator;
				}
			}
		}

		// 가장 높은 주파수 설정으로 변환
		if (update_fps_limit_to_highest_hz)
		{
			double hz = m_vecDisplayModes.front().hz;
			Timer* timer = context->GetSubModule<Timer>();
			if (hz > timer->GetFPSLimit())
			{
				// FPS 최대치도 변경한다.
				timer->SetFPSLimit(hz);
			}
		}
	}

	uint32_t Display::GetWidth()
	{
		return static_cast<uint32_t>(GetSystemMetrics(SM_CXSCREEN));
	}

	uint32_t Display::GetHeight()
	{
		return static_cast<uint32_t>(GetSystemMetrics(SM_CYSCREEN));
	}

	uint32_t Display::GetWidthVirtual()
	{
		return static_cast<uint32_t>(GetSystemMetrics(SM_CXVIRTUALSCREEN));
	}

	uint32_t Display::GetHeightVirtual()
	{
		return static_cast<uint32_t>(GetSystemMetrics(SM_CYVIRTUALSCREEN));
	}
}