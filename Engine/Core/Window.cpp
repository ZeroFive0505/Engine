#include "Common.h"
#include "Window.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include "../Input/Input.h"
#include "Settings.h"
#include "Context.h"
#include "EventSystem.h"

#pragma comment(lib, "Imm32.lib")
#pragma comment(lib, "Setupapi.lib")

using namespace std;

namespace PlayGround
{
	Window::Window(Context* context) : SubModule(context)
	{
		// SDL Video 초기화
        if (SDL_WasInit(SDL_INIT_VIDEO) != 1)
        {
            if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
            {
                LOG_ERROR("Failed to initialise SDL video subsystem: %s.", SDL_GetError());
                return;
            }
        }

        // SDL Events 초기화
        if (SDL_WasInit(SDL_INIT_EVENTS) != 1)
        {
            if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0)
            {
                LOG_ERROR("Failed to initialise SDL events subsystem: %s.", SDL_GetError());
                return;
            }
        }

		uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;
		m_Window = SDL_CreateWindow(
			m_Title.c_str(),
			SDL_WINDOWPOS_UNDEFINED, 
			SDL_WINDOWPOS_UNDEFINED,
			m_Width,
			m_Height,
			flags);

		if (!m_Window)
		{
			LOG_ERROR("Could not create window: %s.", SDL_GetError());
			return;
		}

		string version = to_string(SDL_MAJOR_VERSION) + "." + to_string(SDL_MINOR_VERSION) + "." + to_string(SDL_PATCHLEVEL);
		m_Context->GetSubModule<Settings>()->RegisterThirdParty("SDL", version, "https://github.com/libsdl-org/SDL/releases");
	}

	Window::~Window()
	{
		SDL_DestroyWindow(m_Window);

		SDL_Quit();
	}

	void Window::Update(double delta_time)
	{
		SDL_Event sdl_event;

		while (SDL_PollEvent(&sdl_event))
		{
			if (sdl_event.type == SDL_WINDOWEVENT)
			{
				switch (sdl_event.window.event)
				{
				case SDL_WINDOWEVENT_SHOWN:
					m_Show = true;
					break;
				case SDL_WINDOWEVENT_HIDDEN:
					m_Show = false;
					break;
				case SDL_WINDOWEVENT_EXPOSED:
					// Window has been exposed and should be redrawn
					break;
				case SDL_WINDOWEVENT_MOVED:
					// Window has been moved to data1, data2
					break;
				case SDL_WINDOWEVENT_RESIZED:
					m_Width = static_cast<uint32_t>(sdl_event.window.data1);
					m_Height = static_cast<uint32_t>(sdl_event.window.data2);
					break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					m_Width = static_cast<uint32_t>(sdl_event.window.data1);
					m_Height = static_cast<uint32_t>(sdl_event.window.data2);
					break;
				case SDL_WINDOWEVENT_MINIMIZED:
					m_Minimized = true;
					m_Maximized = false;
					break;
				case SDL_WINDOWEVENT_MAXIMIZED:
					m_Maximized = true;
					m_Minimized = false;
					break;
				case SDL_WINDOWEVENT_RESTORED:
					// SDL_Log("Window %d restored", event->window.windowID);
					break;
				case SDL_WINDOWEVENT_ENTER:
					// Window has gained mouse focus
					break;
				case SDL_WINDOWEVENT_LEAVE:
					// Window has lost mouse focus
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					// Window has gained keyboard focus
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					// Window has lost keyboard focus
					break;
				case SDL_WINDOWEVENT_CLOSE:
					m_Close = true;
					break;
				case SDL_WINDOWEVENT_TAKE_FOCUS:
					// Window is being offered a focus (should SetWindowInputFocus() on itself or a subwindow, or ignore)
					break;
				case SDL_WINDOWEVENT_HIT_TEST:
					//Window had a hit test that wasn't SDL_HITTEST_NORMAL.
					break;
				case SDL_WINDOWEVENT_ICCPROF_CHANGED:
					// The ICC profile of the window's display has changed
					break;
				case SDL_WINDOWEVENT_DISPLAY_CHANGED:
					// Window has been moved to display data1
					break;
				default:
					LOG_ERROR("Unhandled window event");
					break;
				}
			}

			FIRE_EVENT_DATA(EventType::EventSDL, &sdl_event);
		}

		Input* input = m_Context->GetSubModule<Input>();

		if (input)
		{
			if (input->GetKey(EKeyCode::Alt_Right) && input->GetKeyDown(EKeyCode::Enter))
				ToggleFullScreen();
		}
	}

	void Window::Show()
	{
		ASSERT(m_Window != nullptr);

		SDL_ShowWindow(m_Window);
	}

	void Window::Hide()
	{
		ASSERT(m_Window != nullptr);

		SDL_HideWindow(m_Window);
	}

	void Window::Focus()
	{
		ASSERT(m_Window != nullptr);

		SDL_RaiseWindow(m_Window);
	}

	void Window::FullScreen()
	{
		ASSERT(m_Window != nullptr);

		LOG_INFO("Entering full screen mode...");

		SDL_SetWindowFullscreen(m_Window, SDL_WINDOW_FULLSCREEN);
	}

	void Window::Windowed()
	{
		ASSERT(m_Window != nullptr);

		LOG_INFO("Entering windowed mode...");

		SDL_SetWindowFullscreen(m_Window, 0);
		m_FullScreen = false;
	}

	void Window::ToggleFullScreen()
	{
		if (!m_FullScreen)
			FullScreen();
		else
			Windowed();

		FIRE_EVENT(EventType::WindowOnFullScreenToggled);
	}

	void Window::FullScreenBorderless()
	{
		ASSERT(m_Window != nullptr);

		SDL_SetWindowFullscreen(m_Window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		m_FullScreen = true;
	}

	void Window::Minimize()
	{
		ASSERT(m_Window != nullptr);

		SDL_SetWindowFullscreen(m_Window, SDL_WINDOW_MINIMIZED);
	}

	void Window::Maximize()
	{
		ASSERT(m_Window != nullptr);

		SDL_SetWindowFullscreen(m_Window, SDL_WINDOW_MAXIMIZED);
	}

	void Window::SetSize(const uint32_t width, const uint32_t height)
	{
		ASSERT(m_Window != nullptr);

		SDL_SetWindowSize(m_Window, static_cast<int>(width), static_cast<int>(height));
	}

	uint32_t Window::GetWidth()
	{
		ASSERT(m_Window != nullptr);

		int width = 0;
		SDL_GetWindowSize(m_Window, &width, nullptr);
		return static_cast<uint32_t>(width);
	}

	uint32_t Window::GetHeight()
	{
		ASSERT(m_Window != nullptr);

		int height = 0;
		SDL_GetWindowSize(m_Window, nullptr, &height);
		return static_cast<uint32_t>(height);
	}

	void* Window::GetHandle()
	{
		ASSERT(m_Window != nullptr);

		SDL_SysWMinfo sys_info;
		SDL_VERSION(&sys_info.version);
		SDL_GetWindowWMInfo(m_Window, &sys_info);
		return static_cast<void*>(sys_info.info.win.window);
	}
}