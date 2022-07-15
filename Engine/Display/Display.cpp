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
		// ���� �̹� ����Ǿ��ִ� ���÷��� ����� ��ȯ
		for (const sDisplayMode& display : m_vecDisplayModes)
		{
			if (display_mode == display)
				return;
		}

		m_vecDisplayModes.emplace_back(display_mode);

		// ���ļ��� ������������ �����Ѵ�.
		sort(m_vecDisplayModes.begin(), m_vecDisplayModes.end(), [](const sDisplayMode& a, const sDisplayMode& b)
		{
			return a.hz > b.hz;
		});


		for (const sDisplayMode& display_mode : m_vecDisplayModes)
		{
			// ���� ���� ���̰� �� ũ�ٸ�
			if (display_mode.width > m_CurrentDisplayMode.width || display_mode.height > m_CurrentDisplayMode.height)
			{
				// ���ļ��� �� ���ٸ�
				if (display_mode.hz >= m_CurrentDisplayMode.hz)
				{
					// ���� ���÷��� ��带 �ٲ۴�.
					m_CurrentDisplayMode.width = display_mode.width;
					m_CurrentDisplayMode.height = display_mode.height;
					m_CurrentDisplayMode.hz = display_mode.hz;
					m_CurrentDisplayMode.numerator = display_mode.numerator;
					m_CurrentDisplayMode.denominator = display_mode.denominator;
				}
			}
		}

		// ���� ���� ���ļ� �������� ��ȯ
		if (update_fps_limit_to_highest_hz)
		{
			double hz = m_vecDisplayModes.front().hz;
			Timer* timer = context->GetSubModule<Timer>();
			if (hz > timer->GetFPSLimit())
			{
				// FPS �ִ�ġ�� �����Ѵ�.
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