#pragma once

#include "Math/Vector4.h"

// Ä®¶ó ÇÇÄ¿
class ColorPicker
{
public:
	ColorPicker(const std::string& windowTitle);
	void Update();
	inline void SetColor(const PlayGround::Math::Vector4 color) { m_Color = color; }
	inline const PlayGround::Math::Vector4& GetColor() const { return m_Color; }

private:
	void ShowColorPicker();

	bool m_Visible;
	PlayGround::Math::Vector4 m_Color;
	std::string m_WindowTitle;
};

