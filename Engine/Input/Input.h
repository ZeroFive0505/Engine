#pragma once

#include <array>
#include "../Math/Vector2.h"
#include "../Core/SubModule.h"
#include "../Core/Variant.h"


namespace PlayGround
{
	enum class EKeyCode
	{
		// Keyboard
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,
		Alpha0, Alpha1, Alpha2, Alpha3, Alpha4, Alpha5, Alpha6, Alpha7, Alpha8, Alpha9,
		Keypad0, Keypad1, Keypad2, Keypad3, Keypad4, Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
		Q, W, E, R, T, Y, U, I, O, P,
		A, S, D, F, G, H, J, K, L,
		Z, X, C, V, B, N, M,
		Esc,
		Tab,
		Shift_Left, Shift_Right,
		Ctrl_Left, Ctrl_Right,
		Alt_Left, Alt_Right,
		Space,
		CapsLock,
		Backspace,
		Enter,
		Delete,
		Arrow_Left, Arrow_Right, Arrow_Up, Arrow_Down,
		Page_Up, Page_Down,
		Home,
		End,
		Insert,

		// Mouse
		Click_Left,
		Click_Middle,
		Click_Right,

		// Gamepad
		DPad_Up,
		DPad_Down,
		DPad_Left,
		DPad_Right,
		Button_A,
		Button_B,
		Button_X,
		Button_Y,
		Back,
		Guide,
		Start,
		Left_Stick,
		Right_Stick,
		Left_Shoulder,
		Right_Shoulder,
		Misc1,    // Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button
		Paddle1,  // Xbox Elite paddle P1
		Paddle2,  // Xbox Elite paddle P3
		Paddle3,  // Xbox Elite paddle P2
		Paddle4,  // Xbox Elite paddle P4
		Touchpad, // PS4/PS5 touchpad button
	};

	class Input : public SubModule
	{
    public:
        Input(Context* context);
        ~Input() = default;

        //= ISubsystem ========================
        void Update(double delta_time) override;
        void PostUpdate() override;
        //=====================================

        // Polling driven input
        void PollMouse();
        void PollKeyboard();

        // Event driven input
        void OnEvent(const Variant& event_variant);
        void OnEventMouse(void* event_mouse);
        void OnEventController(void* event_controller);

        // Keys
        bool GetKey(const EKeyCode key) { return m_keys[static_cast<uint32_t>(key)]; }                                // Returns true while the button identified by KeyCode is held down.
        bool GetKeyDown(const EKeyCode key) { return GetKey(key) && !m_keys_previous_frame[static_cast<uint32_t>(key)]; } // Returns true during the frame the user pressed down the button identified by KeyCode.
        bool GetKeyUp(const EKeyCode key) { return !GetKey(key) && m_keys_previous_frame[static_cast<uint32_t>(key)]; } // Returns true the first frame the user releases the button identified by KeyCode.

        // Mouse
        void SetMouseCursorVisible(const bool visible);
        bool GetMouseCursorVisible()                            const;
        void SetMouseIsInViewport(const bool is_in_viewport) { m_mouse_is_in_viewport = is_in_viewport; }
        bool GetMouseIsInViewport()                             const { return m_mouse_is_in_viewport; }
        const Math::Vector2& GetMousePosition()                 const { return m_mouse_position; }
        void SetMousePosition(const Math::Vector2& position);
        const Math::Vector2& GetMouseDelta()                    const { return m_mouse_delta; }
        const Math::Vector2& GetMouseWheelDelta()               const { return m_mouse_wheel_delta; }
        void SetEditorViewportOffset(const Math::Vector2& offset) { m_editor_viewport_offset = offset; }
        const Math::Vector2 GetMousePositionRelativeToWindow() const;
        const Math::Vector2 GetMousePositionRelativeToEditorViewport() const;

        // Controller
        bool ControllerIsConnected()                        const { return m_controller_connected; }
        const Math::Vector2& GetControllerThumbStickLeft()  const { return m_controller_thumb_left; }
        const Math::Vector2& GetControllerThumbStickRight() const { return m_controller_thumb_right; }
        float GetControllerTriggerLeft()                    const { return m_controller_trigger_left; }
        float GetControllerTriggerRight()                   const { return m_gamepad_trigger_right; }
        // Vibrate the gamepad.
        // Motor speed range is from 0.0 to 1.0f.
        // The left motor is the low-frequency rumble motor.
        // The right motor is the high-frequency rumble motor. 
        // The two motors are not the same, and they create different vibration effects.
        bool GamepadVibrate(const float left_motor_speed, const float right_motor_speed) const;

    private:
        // Keys
        std::array<bool, 107> m_keys;
        std::array<bool, 107> m_keys_previous_frame;
        uint32_t start_index_mouse = 83;
        uint32_t start_index_gamepad = 86;

        // Mouse
        Math::Vector2 m_mouse_position = Math::Vector2::Zero;
        Math::Vector2 m_mouse_delta = Math::Vector2::Zero;
        Math::Vector2 m_mouse_wheel_delta = Math::Vector2::Zero;
        Math::Vector2 m_editor_viewport_offset = Math::Vector2::Zero;
        bool m_mouse_is_in_viewport = true;

        // Controller
        void* m_controller = nullptr;
        bool m_controller_connected = false;
        Math::Vector2 m_controller_thumb_left = Math::Vector2::Zero;
        Math::Vector2 m_controller_thumb_right = Math::Vector2::Zero;
        float m_controller_trigger_left = 0.0f;
        float m_gamepad_trigger_right = 0.0f;
	};
}

