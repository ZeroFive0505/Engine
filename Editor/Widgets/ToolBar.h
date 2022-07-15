#pragma once

#include "Widget.h"
#include <unordered_map>

enum class EIconType;

// Åø¹Ù À§Á¬
class ToolBar : public Widget
{
public:
	ToolBar(Editor* editor);

	void UpdateAlways() override;

private:
	std::unordered_map<EIconType, Widget*> m_mapWidgets;
	float m_ButtonSize = 16.0f;
};

