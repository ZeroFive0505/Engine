#pragma once

#include "Widget.h"

namespace PlayGround
{
	class Renderer;
}

// ���� ������ �ɼ� ����
class RenderOptions : public Widget
{
public:
	RenderOptions(Editor* editor);

	void UpdateVisible() override;

private:
	PlayGround::Renderer* m_Renderer;
};

