#pragma once

#include "Widget.h"
#include "RHI/RHI_Viewport.h"

namespace PlayGround
{
	class Renderer;
	class Settings;
	class World;
	class Input;
}

// ºä Æ÷Æ® À§Á¬
class Viewport : public Widget
{
public:
	Viewport(Editor* editor);

	void UpdateVisible() override;

private:
	PlayGround::Math::Vector2 m_Offset = PlayGround::Math::Vector2::Zero;
	float m_WindowPaddng = 4.0f;
	bool m_Has_resolution_been_set = false;
	uint8_t m_FramesCount = 0;
	float m_Width = 0.0f;
	float m_Height = 0.0f;
	PlayGround::Renderer* m_Renderer = nullptr;
	PlayGround::Settings* m_Settings = nullptr;
	PlayGround::World* m_World = nullptr;
	PlayGround::Input* m_Input = nullptr;
};

