#pragma once

#include "Widget.h"

// ¸®¼Ò½º ºä¾î
class ResourceViewer : public Widget
{
public:
	ResourceViewer(Editor* editor);

	void UpdateVisible() override;
};

