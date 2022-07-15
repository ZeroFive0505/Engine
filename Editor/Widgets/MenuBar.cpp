#include "MenuBar.h"
#include "Core/Settings.h"
#include "Rendering/Model.h"

using namespace std;
using namespace PlayGround;

// �޴��ٿ��� ���� ���� ������
namespace Widget_MenuBar
{
	static bool g_ShowShortcutsWindow = false;
	static bool g_ShowAboutWindow = false;
	static bool g_FileDialogVisible = false;
	static bool Imgui_metrics = false;
	static bool Imgui_style = false;
	static bool Imgui_demo = false;
	static Input* g_Input = nullptr;
	static World* g_World = nullptr;
	static string g_FileDialogSelection;
}

MenuBar::MenuBar(Editor* editor) : Widget(editor)
{
	// �޴��� �ʱ�ȭ
	m_Title = "MenuBar";
	m_IsWindow = false;
	m_ToolBar = make_unique<ToolBar>(editor);
	m_FileDialog = make_unique<FileDialog>(m_Context, true, FileDialog_Type_FileSelection, FileDialog_Op_Open, FileDialog_Filter_World);
	Widget_MenuBar::g_Input = m_Context->GetSubModule<Input>();
	Widget_MenuBar::g_World = m_Context->GetSubModule<World>();
}

void MenuBar::UpdateAlways()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(GetPadding(), GetPadding()));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	
	// �޴��� ����
	if (ImGui::BeginMainMenuBar())
	{
		// ����
		if (ImGui::BeginMenu("World"))
		{
			// ���ο� ���� ���� �޴�
			if (ImGui::MenuItem("New"))
			{
				m_Context->GetSubModule<World>()->New();
			}

			ImGui::Separator();

			// ���� �ε� �޴�
			if (ImGui::MenuItem("Load"))
			{
				ShowWorldLoadDialog();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save", "Ctrl+S"))
			{
				ShowWorldSaveDialog();
			}

			if (ImGui::MenuItem("Save As...", "Ctrl+S"))
			{
				ShowWorldSaveDialog();
			}

			ImGui::EndMenu();
		}

		/*if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("ImGui Metrics", nullptr, &Widget_MenuBar::Imgui_metrics);
			ImGui::MenuItem("ImGui Style", nullptr, &Widget_MenuBar::Imgui_style);
			ImGui::MenuItem("ImGui Demo", nullptr, &Widget_MenuBar::Imgui_demo);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("About", nullptr, &Widget_MenuBar::g_ShowAboutWindow);
			ImGui::MenuItem("Shortcuts & Input Reference", "Ctrl+P", &Widget_MenuBar::g_ShowShortcutsWindow);
			ImGui::EndMenu();
		}*/

		ImGui::Spacing();
		m_ToolBar->Update();

		ImGui::EndMainMenuBar();

	}
		
	ImGui::PopStyleVar(2);

	/*if (Widget_MenuBar::Imgui_metrics)
	{
		ImGui::ShowMetricsWindow();
	}

	if (Widget_MenuBar::Imgui_style)
	{
		ImGui::Begin("Style Editor", nullptr, ImGuiWindowFlags_NoDocking);
		ImGui::ShowStyleEditor();
		ImGui::End();
	}

	if (Widget_MenuBar::Imgui_demo)
	{
		ImGui::ShowDemoWindow(&Widget_MenuBar::Imgui_demo);
	}*/

	// ����Ű ó��
	HandleKeyShortcuts();
	// ���� ���̾�α�
	DrawFileDialog();
	// DrawAboutWindow();
	// DrawShortCutsWindow();
}

void MenuBar::HandleKeyShortcuts() const
{
	if (Widget_MenuBar::g_Input->GetKey(EKeyCode::Ctrl_Left) && Widget_MenuBar::g_Input->GetKeyDown(EKeyCode::P))
	{
		Widget_MenuBar::g_ShowShortcutsWindow = !Widget_MenuBar::g_ShowShortcutsWindow;
	}
}

void MenuBar::ShowWorldSaveDialog()
{
	m_FileDialog->SetOperation(FileDialog_Op_Save);
	Widget_MenuBar::g_FileDialogVisible = true;
}

void MenuBar::ShowWorldLoadDialog()
{
	m_FileDialog->SetOperation(FileDialog_Op_Load);
	Widget_MenuBar::g_FileDialogVisible = true;
}


void MenuBar::DrawFileDialog() const
{
	if (Widget_MenuBar::g_FileDialogVisible)
	{
		ImGui::SetNextWindowFocus();
	}

	if (m_FileDialog->Show(&Widget_MenuBar::g_FileDialogVisible, nullptr, &Widget_MenuBar::g_FileDialogSelection))
	{
		if (m_FileDialog->GetOperation() == FileDialog_Op_Open || m_FileDialog->GetOperation() == FileDialog_Op_Load)
		{
			if (FileSystem::IsEngineSceneFile(Widget_MenuBar::g_FileDialogSelection))
			{
				EditorHelper::Get().LoadWorld(Widget_MenuBar::g_FileDialogSelection);
				Widget_MenuBar::g_FileDialogVisible = false;
			}
		}

		else if (m_FileDialog->GetOperation() == FileDialog_Op_Save)
		{
			if (m_FileDialog->GetFilter() == FileDialog_Filter_World)
			{
				EditorHelper::Get().SaveWorld(Widget_MenuBar::g_FileDialogSelection);
				Widget_MenuBar::g_FileDialogVisible = false;
			}
		}
	}
}

void MenuBar::DrawShortCutsWindow() const
{
}

void MenuBar::DrawAboutWindow() const
{
}
