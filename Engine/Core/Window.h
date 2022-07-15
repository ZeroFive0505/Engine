#pragma once

#include <string>
#include <functional>
#include "Engine.h"
#include "SubModule.h"

struct SDL_Window;
typedef union SDL_Event SDL_Event;

/// <summary>
/// SDL을 이용한 윈도우 생성 클래스
/// </summary>

namespace PlayGround
{
	class Window : public SubModule
	{
	public:
		Window(Context* context);
		~Window();

		void Update(double delta_time) override;

		void Show();
		void Hide();
		void Focus();
		void FullScreen();
		void Windowed();
		void ToggleFullScreen();
		void FullScreenBorderless();
		void Minimize();
		void Maximize();
		void SetSize(const uint32_t width, const uint32_t height);
		uint32_t GetWidth();
		uint32_t GetHeight();

		void* GetHandle();

		// 핸들 반환
		inline void* GetHandleSDL() const
		{
			return m_Window;
		}

		// 윈도우 닫기
		inline bool WantsToClose() const
		{
			return m_Close;
		}

		// 최소화
		inline bool IsMinimized() const
		{
			return m_Minimized;
		}

		// 풀 스크린
		inline bool IsFullScreen() const
		{
			return m_FullScreen;
		}

	private:
		std::string m_Title = "PlayGround";
		uint32_t m_Width = 1024;
		uint32_t m_Height = 768;
		bool m_Show = false;
		bool m_Minimized = false;
		bool m_Maximized = false;
		bool m_Close = false;
		bool m_FullScreen = false;
		SDL_Window* m_Window = nullptr;
	};
}

