#pragma once

#include "Widget.h"
#include <memory>
#include "ToolBar.h"
#include "FileDialog.h"

namespace PlayGround
{
	class Context;
}

class ToolBar;
class FileDialog;

// ��� �޴���
class MenuBar : public Widget
{
public:
	MenuBar(Editor* editor);

	void UpdateAlways() override;
	void ShowWorldSaveDialog();
	void ShowWorldLoadDialog();

	inline static const float GetPadding() { return 8.0f; }

private:
	void DrawFileDialog() const;
	void DrawAboutWindow() const;
	void DrawShortCutsWindow() const;
	void HandleKeyShortcuts() const;

	// �޴����� ������
	std::unique_ptr<ToolBar> m_ToolBar;
	std::unique_ptr<FileDialog> m_FileDialog;
};

