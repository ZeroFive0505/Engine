#include "Viewport.h"
#include "Core/Timer.h"
#include "Core/Settings.h"
#include "Rendering/Model.h"
#include "ImGuiHelper.h"
#include "../Editor.h"
#include "WorldViewer.h"

using namespace std;
using namespace PlayGround;
using namespace PlayGround::Math;

Viewport::Viewport(Editor* editor) : Widget(editor)
{
	// 뷰 포트 위젯 초기화
	m_Title = "Viewport";
	m_InitSize = Vector2(400.0f, 250.0f);
	m_Flags |= ImGuiWindowFlags_NoScrollbar;
	m_Padding = Vector2(2.0f);
	m_World = m_Context->GetSubModule<World>();
	m_Renderer = m_Context->GetSubModule<Renderer>();
	m_Settings = m_Context->GetSubModule<Settings>();
	m_Input = m_Context->GetSubModule<Input>();
}

void Viewport::UpdateVisible()
{
	// 렌더러가 아직 초기화 안되있을시에는 그냥 반환
	if (!m_Renderer)
		return;

	// 높이 너비 계산
	float width = static_cast<float>(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
	float height = static_cast<float>(ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);

	// 뷰 포트늬 높이 너비와 현재 높이 너비가 다르면 설정
	if (m_Width != width || m_Height != height)
	{
		m_Renderer->SetViewport(width, height);

		m_Width = width;
		m_Height = height;
	}

	// 뷰 포트 오프셋 설정(피킹 정확도를 위해서)
	Vector2 offset = Vector2(ImGui::GetCursorPos().x, ImGui::GetCursorPos().y);
	offset.y += 34.0f;
	if (WorldViewer* viewer = m_Editor->GetWidget<WorldViewer>())
		offset.x += viewer->GetWidth();
	m_Input->SetEditorViewportOffset(offset);
	
	if (!m_Context->GetSubModule<Settings>()->HasLoadedUserSettings() && !m_Has_resolution_been_set && m_FramesCount > 2)
	{
		m_Renderer->SetResolutionRender(static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height));
		m_Renderer->SetResolutionOutput(static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height));
		m_Has_resolution_been_set = true;
	}

	m_FramesCount++;

	// 백버퍼에 렌더링된 최종 이미지를 가져와서 그려낸다.
	ImGuiEX::Image(m_Renderer->GetFrameTexture(), ImVec2(static_cast<float>(m_Width), static_cast<float>(m_Height)));

	// 현재 마우스가 뷰 포트 안에 존재하는지
	m_Input->SetMouseIsInViewport(ImGui::IsItemHovered());

	// 클릭 여부 클릭시 피킹된 엔티티를 반환한다.
	if (ImGui::IsMouseClicked(0) && ImGui::IsItemHovered())
		EditorHelper::Get().PickEntity();

	if (auto payload = ImGuiEX::ReceiveDragPayload(ImGuiEX::DragPayloadType::DragPayload_Model))
		EditorHelper::Get().LoadModel(get<const char*>(payload->data));
}