#pragma once

#include <string>
#include <functional>
#include "../ImGui/Source/imgui.h"

struct ImGuiWindow;
class Editor;

namespace PlayGround
{
	class Context;
	class Profiler;
}

const float widget_default_propery = -1.0f;
const float widget_position_screen_center = -2.0f;

// 위젯 기본 베이스 클래스
class Widget
{
public:
	Widget(Editor* editor);
	virtual ~Widget() = default;

	void Update();

	// 위젯 베이스 가상 메서드
	virtual void UpdateAlways();
	virtual void UpdateVisible();
	virtual void OnShow();
	virtual void OnHide();
	virtual void OnPushStyleVar();

	template <typename T>
	void PushStyleVar(ImGuiStyleVar idx, T val)
	{
		ImGui::PushStyleVar(idx, val);
		m_VarPushes++;
	}

	inline float GetHeight() const { return m_Height; }
	inline float GetWidth() const { return m_Width; }
	inline ImGuiWindow* GetWindow() const { return m_Window; }
	inline const std::string& GetTitle() const { return m_Title; }
	inline bool& GetVisible() { return m_Visible; }
	inline void SetVisible(bool is_visible) { m_Visible = is_visible; }

protected:
	bool m_IsWindow = true;
	bool m_Visible = true;
	int m_Flags = ImGuiWindowFlags_NoCollapse;
	float m_Height = 0.0f;
	float m_Width = 0.0f;
	float m_Alpha = -1.0f;
	PlayGround::Math::Vector2 m_Position = widget_default_propery;
	PlayGround::Math::Vector2 m_InitSize = widget_default_propery;
	PlayGround::Math::Vector2 m_MinSize = widget_default_propery;
	PlayGround::Math::Vector2 m_MaxSize = FLT_MAX;
	PlayGround::Math::Vector2 m_Padding = widget_default_propery;
	Editor* m_Editor = nullptr;
	PlayGround::Context* m_Context;
	PlayGround::Profiler* m_Profiler = nullptr;
	ImGuiWindow* m_Window = nullptr;
	std::string m_Title = "Title";

private:
	uint8_t m_VarPushes = 0;
};

