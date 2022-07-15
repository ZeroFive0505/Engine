#pragma once

#include "Widget.h"

namespace PlayGround
{
	class Renderer;
}

// ÅØ½ºÃÄ ºä¾î À§Á¬
class TextureViewer : public Widget
{
public:
	TextureViewer(Editor* editor);

	void UpdateVisible() override;

private:
	PlayGround::Renderer* m_Renderer = nullptr;
	uint32_t m_TextureIndex = 1;
	bool m_Magnifying_glass = false;
	bool m_ChannelR = true;
	bool m_ChannelG = true;
	bool m_ChannelB = true;
	bool m_ChannelA = true;
	bool m_GammaCorrect = false;
	bool m_Pack = false;
	bool m_Boost = false;
	bool m_Abs = false;
	bool m_PointSampling = false;
};

