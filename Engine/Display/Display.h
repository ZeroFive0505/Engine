#pragma once

#include "DisplayMode.h"

namespace PlayGround
{
	class Context;

	// 현재 디스플레이 상태를 저장하는 클래스
	class Display
	{
	public:
		Display() = default;

		// 디스플레이 모드 설정
		static void RegisterDisplayMode(const sDisplayMode& display_mode, const bool update_fps_limit_to_highest_hz, Context* context);
		
		// 디스플레이 모드 변경
		inline static void SetActiveDisplayMode(const sDisplayMode& display_mode) { m_CurrentDisplayMode = display_mode; }

		// 현재 디스플레이 모드 반환
		inline static const sDisplayMode& GetActiveDisplayMode() { return m_CurrentDisplayMode; }

		// 저장된 모든 디스플레이 모드 반환
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