#pragma once

#include "Widget.h"

// ���ҽ� ���
class ResourceViewer : public Widget
{
public:
	ResourceViewer(Editor* editor);

	void UpdateVisible() override;
};

