#pragma once

#include "DisplayMode.h"

namespace PlayGround
{
	class Context;

	// ���� ���÷��� ���¸� �����ϴ� Ŭ����
	class Display
	{
	public:
		Display() = default;

		// ���÷��� ��� ����
		static void RegisterDisplayMode(const sDisplayMode& display_mode, const bool update_fps_limit_to_highest_hz, Context* context);
		
		// ���÷��� ��� ����
		inline static void SetActiveDisplayMode(const sDisplayMode& display_mode) { m_CurrentDisplayMode = display_mode; }

		// ���� ���÷��� ��� ��ȯ
		inline static const sDisplayMode& GetActiveDisplayMode() { return m_CurrentDisplayMode; }

		// ����� ��� ���÷��� ��� ��ȯ
		inline static const std::vector<sDisplayMode>& GetDisplayModes() { return m_vecDisplayModes; }

		static uint32_t GetWidth();
		static uint32_t GetHeight();

		static uint32_t GetWidthVirtual();
		static uint32_t GetHeightVirtual();

	private:
		static std::vector<sDisplayMode> m_vecDisplayModes;
		static sDisplayMode m_CurrentDisplayMode;
	};
}