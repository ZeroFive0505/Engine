#include "ToolBar.h"
#include "Profiler.h"
#include "ResourceViewer.h"
#include "RenderOptions.h"
#include "TextureViewer.h"
#include "Core/Engine.h"
#include "Rendering/Model.h"
#include "ImGuiHelper.h"
#include "../ImGui/Source/imgui_internal.h"
#include "../IconProvider.h"
#include "../Editor.h"
#include "Display/Display.h"
#include "World/World.h"

using namespace std;
using namespace PlayGround::Math;

ToolBar::ToolBar(Editor* editor) : Widget(editor)
{
	// 툴바 위젯 초기화

	m_Title = "ToolBar";
	m_IsWindow = false;

	m_Flags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoTitleBar;

	m_mapWidgets[EIconType::Component_Options] = m_Editor->GetWidget<RenderOptions>();

	m_Context->m_Engine->DisableEngineMode(PlayGround::EEngine_Mode::GameMode);
}

void ToolBar::UpdateAlways()
{
	// 버튼 람다 함수
	auto show_button = [this](EIconType icon_type,
		const function<bool()>& get_visibility,
		const function<void()>& make_visible)
	{
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, get_visibility() ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_Button]);

		if (ImGuiEX::ImageButton(icon_type, m_ButtonSize))
			make_visible();

		ImGui::PopStyleColor();
	};

	// 툴바에 추가된 위젯들을 모두 순회하면서 렌더링한다.
	for (auto& widget : m_mapWidgets)
	{
		Widget* w = widget.second;
		const EIconType widget_icon = widget.first;

		// 버튼 렌더
		show_button(widget_icon,
			[this, &w]()
		{
			return w->GetVisible();
		},
			[this, &w]()
		{
			return w->SetVisible(true);
		});
	}

	show_button(EIconType::Button_Play, [this]() {
		return m_Context->m_Engine->IsEngineModeSet(PlayGround::EEngine_Mode::GameMode);
	},
		[this]() {
		m_Context->m_Engine->ToggleEngineMode(PlayGround::EEngine_Mode::GameMode);
	});

	show_button(EIconType::Button_Pause, [this]()
	{
		return m_Context->m_Engine->IsEngineModeSet(PlayGround::EEngine_Mode::PauseMode);
	},
		[this]() {
		m_Context->m_Engine->ToggleEngineMode(PlayGround::EEngine_Mode::PauseMode);
	});

	show_button(EIconType::Button_StepForward, [this]()
	{
		if (m_Context->m_Engine->IsEngineModeSet(PlayGround::EEngine_Mode::PauseMode))
			return false;
		else
			return true;
	},
		[this]() {
		m_Context->GetSubModule<PlayGround::World>()->SetUpdateOnce();
	});

}