#include "Common.h"
#include "Input.h"
#include "SDL.h"
#include "../Core/EventSystem.h"
#include "../Core/Window.h"
#include "../Core/Context.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	Input::Input(Context* context) : SubModule(context)
	{
		if (SDL_WasInit(SDL_INIT_EVENTS) != 1)
		{
			if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0)
			{
				LOG_ERROR("Failed to initialize SDL events subsystem: %s.", SDL_GetError());
				return;
			}
		}

		if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) != 1)
		{
			if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
			{
				LOG_ERROR("Failed to initialize SDL events subsystem: %s.", SDL_GetError());
				return;
			}
		}

		m_keys.fill(false);
		m_keys_previous_frame.fill(false);

		SUBSCRIBE_TO_EVENT(EventType::EventSDL, EVENT_HANDLER_VARIANT(OnEvent));
	}

	void Input::Update(double delta_time)
	{
		m_keys_previous_frame = m_keys;

		PollMouse();
		PollKeyboard();
	}

	void Input::PostUpdate()
	{
		m_mouse_wheel_delta = Vector2::Zero;
	}

	void Input::OnEvent(const Variant& event_variant)
	{
		SDL_Event* event_sdl = event_variant.Get<SDL_Event*>();
		Uint32 event_type = event_sdl->type;

		if (event_type == SDL_MOUSEWHEEL)
			OnEventMouse(event_sdl);

		if (event_type == SDL_CONTROLLERAXISMOTION ||
			event_type == SDL_CONTROLLERBUTTONDOWN ||
			event_type == SDL_CONTROLLERBUTTONUP ||
			event_type == SDL_CONTROLLERDEVICEADDED ||
			event_type == SDL_CONTROLLERDEVICEREMOVED ||
			event_type == SDL_CONTROLLERDEVICEREMAPPED ||
			event_type == SDL_CONTROLLERTOUCHPADDOWN ||
			event_type == SDL_CONTROLLERTOUCHPADMOTION ||
			event_type == SDL_CONTROLLERTOUCHPADUP ||
			event_type == SDL_CONTROLLERSENSORUPDATE)
		{
			OnEventController(event_sdl);
		}
	}

	///////////////////////////////////////////////////////////////////////
	void Input::OnEventController(void* event_controller)
	{
		ASSERT(event_controller != nullptr);
		SDL_Event* sdl_event = static_cast<SDL_Event*>(event_controller);
		Uint32 event_type = sdl_event->type;

		if (!m_controller_connected)
		{
			int size = SDL_NumJoysticks();
			for (int i = 0; i < size; i++)
			{
				if (SDL_IsGameController(i))
				{
					SDL_GameController* controller = SDL_GameControllerOpen(i);
					if (SDL_GameControllerGetAttached(controller) == 1)
					{
						m_controller = controller;
						m_controller_connected = true;
					}
					else
					{
						LOG_ERROR("Failed to get controller: %s.", SDL_GetError());
					}
				}
			}

			SDL_GameControllerEventState(SDL_ENABLE);
		}

		if (event_type == SDL_CONTROLLERDEVICEADDED)
		{
			int size = SDL_NumJoysticks();

			for (int i = 0; i < size; i++)
			{
				if (SDL_IsGameController(i))
				{
					SDL_GameController* controller = SDL_GameControllerOpen(i);
					if (SDL_GameControllerGetAttached(controller) == SDL_TRUE)
					{
						m_controller = controller;
						m_controller_connected = true;
						break;
					}
				}
			}

			if (m_controller_connected)
			{
				LOG_INFO("Controller connected.");
			}
			else
			{
				LOG_ERROR("Failed to get controller: %s.", SDL_GetError());
			}
		}


		if (event_type == SDL_CONTROLLERDEVICEREMOVED)
		{
			m_controller = nullptr;
			m_controller_connected = false;
			LOG_INFO("Controller disconnected.");
		}

		if (event_type == SDL_CONTROLLERBUTTONDOWN)
		{
			Uint8 button = sdl_event->cbutton.button;


			m_keys[start_index_gamepad] = button == SDL_CONTROLLER_BUTTON_DPAD_UP;
			m_keys[start_index_gamepad + 1] = button == SDL_CONTROLLER_BUTTON_DPAD_DOWN;
			m_keys[start_index_gamepad + 2] = button == SDL_CONTROLLER_BUTTON_DPAD_LEFT;
			m_keys[start_index_gamepad + 3] = button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
			m_keys[start_index_gamepad + 4] = button == SDL_CONTROLLER_BUTTON_A;
			m_keys[start_index_gamepad + 5] = button == SDL_CONTROLLER_BUTTON_B;
			m_keys[start_index_gamepad + 6] = button == SDL_CONTROLLER_BUTTON_X;
			m_keys[start_index_gamepad + 7] = button == SDL_CONTROLLER_BUTTON_Y;
			m_keys[start_index_gamepad + 8] = button == SDL_CONTROLLER_BUTTON_BACK;
			m_keys[start_index_gamepad + 9] = button == SDL_CONTROLLER_BUTTON_GUIDE;
			m_keys[start_index_gamepad + 10] = button == SDL_CONTROLLER_BUTTON_START;
			m_keys[start_index_gamepad + 11] = button == SDL_CONTROLLER_BUTTON_LEFTSTICK;
			m_keys[start_index_gamepad + 12] = button == SDL_CONTROLLER_BUTTON_RIGHTSTICK;
			m_keys[start_index_gamepad + 13] = button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
			m_keys[start_index_gamepad + 14] = button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
			m_keys[start_index_gamepad + 15] = button == SDL_CONTROLLER_BUTTON_MISC1;
			m_keys[start_index_gamepad + 16] = button == SDL_CONTROLLER_BUTTON_PADDLE1;
			m_keys[start_index_gamepad + 17] = button == SDL_CONTROLLER_BUTTON_PADDLE2;
			m_keys[start_index_gamepad + 18] = button == SDL_CONTROLLER_BUTTON_PADDLE3;
			m_keys[start_index_gamepad + 19] = button == SDL_CONTROLLER_BUTTON_PADDLE4;
			m_keys[start_index_gamepad + 20] = button == SDL_CONTROLLER_BUTTON_TOUCHPAD;
		}
		else
		{
			for (int i = start_index_gamepad; i <= start_index_gamepad + 11; i++)
			{
				m_keys[i] = false;
			}
		}

		if (event_type == SDL_CONTROLLERAXISMOTION)
		{
			SDL_ControllerAxisEvent event_axis = sdl_event->caxis;

			switch (event_axis.axis)
			{
			case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
				m_controller_trigger_left = static_cast<float>(event_axis.value) / 32768.0f;
				break;
			case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
				m_gamepad_trigger_right = static_cast<float>(event_axis.value) / 32768.0f;
				break;
			case SDL_CONTROLLER_AXIS_LEFTX:
				m_controller_thumb_left.x = static_cast<float>(event_axis.value) / 32768.0f;
				break;
			case SDL_CONTROLLER_AXIS_LEFTY:
				m_controller_thumb_left.y = static_cast<float>(event_axis.value) / 32768.0f;
				break;
			case SDL_CONTROLLER_AXIS_RIGHTX:
				m_controller_thumb_right.x = static_cast<float>(event_axis.value) / 32768.0f;
				break;
			case SDL_CONTROLLER_AXIS_RIGHTY:
				m_controller_thumb_right.y = static_cast<float>(event_axis.value) / 32768.0f;
				break;
			}
		}
	}

	bool Input::GamepadVibrate(const float left_motor_speed, const float right_motor_speed) const
	{
		if (!m_controller_connected)
			return false;

		Uint16 low_frequency_rumble = static_cast<uint16_t>(Util::Clamp(0.0f, 1.0f, left_motor_speed) * 65535);
		Uint16 high_frequency_rumble = static_cast<uint16_t>(Util::Clamp(0.0f, 1.0f, right_motor_speed) * 65535);
		Uint32 duration_ms = 0xFFFFFFFF;

		if (SDL_GameControllerRumble(static_cast<SDL_GameController*>(m_controller), low_frequency_rumble, high_frequency_rumble, duration_ms) == -1)
		{
			LOG_ERROR("Failed to vibrate controller");
			return false;
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////
	void Input::PollKeyboard()
	{
		const Uint8* keys_states = SDL_GetKeyboardState(nullptr);

		// Function
		m_keys[0] = keys_states[SDL_SCANCODE_F1];
		m_keys[1] = keys_states[SDL_SCANCODE_F2];
		m_keys[2] = keys_states[SDL_SCANCODE_F3];
		m_keys[3] = keys_states[SDL_SCANCODE_F4];
		m_keys[4] = keys_states[SDL_SCANCODE_F5];
		m_keys[5] = keys_states[SDL_SCANCODE_F6];
		m_keys[6] = keys_states[SDL_SCANCODE_F7];
		m_keys[7] = keys_states[SDL_SCANCODE_F8];
		m_keys[8] = keys_states[SDL_SCANCODE_F9];
		m_keys[9] = keys_states[SDL_SCANCODE_F10];
		m_keys[10] = keys_states[SDL_SCANCODE_F11];
		m_keys[11] = keys_states[SDL_SCANCODE_F12];
		m_keys[12] = keys_states[SDL_SCANCODE_F13];
		m_keys[13] = keys_states[SDL_SCANCODE_F14];
		m_keys[14] = keys_states[SDL_SCANCODE_F15];
		// Numbers
		m_keys[15] = keys_states[SDL_SCANCODE_0];
		m_keys[16] = keys_states[SDL_SCANCODE_1];
		m_keys[17] = keys_states[SDL_SCANCODE_2];
		m_keys[18] = keys_states[SDL_SCANCODE_3];
		m_keys[19] = keys_states[SDL_SCANCODE_4];
		m_keys[20] = keys_states[SDL_SCANCODE_5];
		m_keys[21] = keys_states[SDL_SCANCODE_6];
		m_keys[22] = keys_states[SDL_SCANCODE_7];
		m_keys[23] = keys_states[SDL_SCANCODE_8];
		m_keys[24] = keys_states[SDL_SCANCODE_9];
		// Keypad
		m_keys[25] = keys_states[SDL_SCANCODE_KP_0];
		m_keys[26] = keys_states[SDL_SCANCODE_KP_1];
		m_keys[27] = keys_states[SDL_SCANCODE_KP_2];
		m_keys[28] = keys_states[SDL_SCANCODE_KP_3];
		m_keys[29] = keys_states[SDL_SCANCODE_KP_4];
		m_keys[30] = keys_states[SDL_SCANCODE_KP_5];
		m_keys[31] = keys_states[SDL_SCANCODE_KP_6];
		m_keys[32] = keys_states[SDL_SCANCODE_KP_7];
		m_keys[33] = keys_states[SDL_SCANCODE_KP_8];
		m_keys[34] = keys_states[SDL_SCANCODE_KP_9];
		// Letters
		m_keys[35] = keys_states[SDL_SCANCODE_Q];
		m_keys[36] = keys_states[SDL_SCANCODE_W];
		m_keys[37] = keys_states[SDL_SCANCODE_E];
		m_keys[38] = keys_states[SDL_SCANCODE_R];
		m_keys[39] = keys_states[SDL_SCANCODE_T];
		m_keys[40] = keys_states[SDL_SCANCODE_Y];
		m_keys[41] = keys_states[SDL_SCANCODE_U];
		m_keys[42] = keys_states[SDL_SCANCODE_I];
		m_keys[43] = keys_states[SDL_SCANCODE_O];
		m_keys[44] = keys_states[SDL_SCANCODE_P];
		m_keys[45] = keys_states[SDL_SCANCODE_A];
		m_keys[46] = keys_states[SDL_SCANCODE_S];
		m_keys[47] = keys_states[SDL_SCANCODE_D];
		m_keys[48] = keys_states[SDL_SCANCODE_F];
		m_keys[49] = keys_states[SDL_SCANCODE_G];
		m_keys[50] = keys_states[SDL_SCANCODE_H];
		m_keys[51] = keys_states[SDL_SCANCODE_J];
		m_keys[52] = keys_states[SDL_SCANCODE_K];
		m_keys[53] = keys_states[SDL_SCANCODE_L];
		m_keys[54] = keys_states[SDL_SCANCODE_Z];
		m_keys[55] = keys_states[SDL_SCANCODE_X];
		m_keys[56] = keys_states[SDL_SCANCODE_C];
		m_keys[57] = keys_states[SDL_SCANCODE_V];
		m_keys[58] = keys_states[SDL_SCANCODE_B];
		m_keys[59] = keys_states[SDL_SCANCODE_N];
		m_keys[60] = keys_states[SDL_SCANCODE_M];
		// Controls
		m_keys[61] = keys_states[SDL_SCANCODE_ESCAPE];
		m_keys[62] = keys_states[SDL_SCANCODE_TAB];
		m_keys[63] = keys_states[SDL_SCANCODE_LSHIFT];
		m_keys[64] = keys_states[SDL_SCANCODE_RSHIFT];
		m_keys[65] = keys_states[SDL_SCANCODE_LCTRL];
		m_keys[66] = keys_states[SDL_SCANCODE_RCTRL];
		m_keys[67] = keys_states[SDL_SCANCODE_LALT];
		m_keys[68] = keys_states[SDL_SCANCODE_RALT];
		m_keys[69] = keys_states[SDL_SCANCODE_SPACE];
		m_keys[70] = keys_states[SDL_SCANCODE_CAPSLOCK];
		m_keys[71] = keys_states[SDL_SCANCODE_BACKSPACE];
		m_keys[72] = keys_states[SDL_SCANCODE_RETURN];
		m_keys[73] = keys_states[SDL_SCANCODE_DELETE];
		m_keys[74] = keys_states[SDL_SCANCODE_LEFT];
		m_keys[75] = keys_states[SDL_SCANCODE_RIGHT];
		m_keys[76] = keys_states[SDL_SCANCODE_UP];
		m_keys[77] = keys_states[SDL_SCANCODE_DOWN];
		m_keys[78] = keys_states[SDL_SCANCODE_PAGEUP];
		m_keys[79] = keys_states[SDL_SCANCODE_PAGEDOWN];
		m_keys[80] = keys_states[SDL_SCANCODE_HOME];
		m_keys[81] = keys_states[SDL_SCANCODE_END];
		m_keys[82] = keys_states[SDL_SCANCODE_INSERT];
	}
	////////////////////////////////////////////////////////////////////
	void Input::PollMouse()
	{
		int x, y;
		Uint32 keys_states = SDL_GetGlobalMouseState(&x, &y);
		Vector2 position = Vector2(static_cast<float>(x), static_cast<float>(y));

		m_mouse_delta = position - m_mouse_position;

		m_mouse_position = position;

		m_keys[start_index_mouse] = (keys_states & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;    // 왼쪽 클릭
		m_keys[start_index_mouse + 1] = (keys_states & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;  // 가운데 클릭
		m_keys[start_index_mouse + 2] = (keys_states & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;   // 오른쪽 클릭
	}

	void Input::OnEventMouse(void* event_mouse)
	{
		ASSERT(event_mouse != nullptr);
		SDL_Event* sdl_event = static_cast<SDL_Event*>(event_mouse);
		Uint32 event_type = sdl_event->type;

		if (event_type == SDL_MOUSEWHEEL)
		{
			if (sdl_event->wheel.x > 0) m_mouse_wheel_delta.x += 1;
			if (sdl_event->wheel.x < 0) m_mouse_wheel_delta.x -= 1;
			if (sdl_event->wheel.y > 0) m_mouse_wheel_delta.y += 1;
			if (sdl_event->wheel.y < 0) m_mouse_wheel_delta.y -= 1;
		}
	}

	bool Input::GetMouseCursorVisible() const
	{
		return SDL_ShowCursor(SDL_QUERY) == 1;
	}

	void Input::SetMouseCursorVisible(const bool visible)
	{
		if (visible == GetMouseCursorVisible())
			return;

		if (static_cast<bool>(SDL_ShowCursor(static_cast<int>(visible))) == visible)
		{
			LOG_ERROR("Failed to change cursor visibility");
		}
	}

	void Input::SetMousePosition(const Math::Vector2& position)
	{
		if (SDL_WarpMouseGlobal(static_cast<int>(position.x), static_cast<int>(position.y)) != 0)
		{
			LOG_ERROR("Failed to set mouse position.");
			return;
		}

		m_mouse_position = position;
	}

	const PlayGround::Math::Vector2 Input::GetMousePositionRelativeToWindow() const
	{
		SDL_Window* window = static_cast<SDL_Window*>(m_Context->GetSubModule<Window>()->GetHandleSDL());
		int window_x, window_y;
		SDL_GetWindowPosition(window, &window_x, &window_y);
		return Vector2(static_cast<float>(m_mouse_position.x - window_x), static_cast<float>(m_mouse_position.y - window_y));
	}

	const PlayGround::Math::Vector2 Input::GetMousePositionRelativeToEditorViewport() const
	{
		return GetMousePositionRelativeToWindow() - m_editor_viewport_offset;
	}
}