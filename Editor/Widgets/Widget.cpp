#include "Widget.h"
#include "../ImGui/Source/imgui_internal.h"
#include "Core/Context.h"
#include "Profiling/Profiler.h"
#include "Display/Display.h"
#include "../Editor.h"

Widget::Widget(Editor* editor)
{
	m_Editor = editor;
	m_Context = editor->GetContext();
	m_Profiler = m_Context->GetSubModule<PlayGround::Profiler>();
	m_Window = nullptr;
}

void Widget::UpdateAlways()
{

}

void Widget::UpdateVisible()
{

}

void Widget::OnShow()
{

}

void Widget::OnHide()
{

}

void Widget::OnPushStyleVar()
{

}


void Widget::Update()
{
	UpdateAlways();

	if (!m_IsWindow)
		return;

	bool begun = false;
	
	{
		if (!m_Visible)
			return;
			
		// �������ϸ� ����
		TIME_BLOCK_START_NAMED(m_Profiler, m_Title.c_str());
	
		// �ʱ� ������
		if (m_InitSize != widget_default_propery)
		{
			ImGui::SetNextWindowSize(m_InitSize, ImGuiCond_FirstUseEver);
		}

		// ������ Ŭ����
		if (m_MinSize != widget_default_propery || m_MaxSize != FLT_MAX)
		{
			ImGui::SetNextWindowSizeConstraints(m_MinSize, m_MaxSize);
		}

		// �е�
		if (m_Padding != widget_default_propery)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, m_Padding);
			m_VarPushes++;
		}

		// ����
		if (m_Alpha != widget_default_propery)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_Alpha);
			m_VarPushes++;
		}

		// ��ġ
		if (m_Position != widget_default_propery)
		{
			if (m_Position == widget_position_screen_center)
			{
				m_Position.x = PlayGround::Display::GetWidth() * 0.5f;
				m_Position.y = PlayGround::Display::GetHeight() * 0.5f;
			}

			ImVec2 pivot_center = ImVec2(0.5f, 0.5f);
			ImGui::SetNextWindowPos(m_Position, ImGuiCond_FirstUseEver, pivot_center);
		}

		// �ݹ�
		OnPushStyleVar();

		// ImGui ����
		if (ImGui::Begin(m_Title.c_str(), &m_Visible, m_Flags))
		{
			m_Window = ImGui::GetCurrentWindow();
			m_Height = ImGui::GetWindowHeight();
			m_Width = ImGui::GetWindowWidth();
			begun = true;
		}
		else if (m_Window && m_Window->Hidden)
		{
			begun = true;
		}

		if (m_Window && m_Window->Appearing)
			OnShow();
		else if (!m_Visible)
			OnHide();
	}

	if (begun)
	{
		UpdateVisible();

		{
			ImGui::End();

			ImGui::PopStyleVar(m_VarPushes);
			m_VarPushes = 0;

			TIME_BLOCK_END(m_Profiler);
		}
	}
}