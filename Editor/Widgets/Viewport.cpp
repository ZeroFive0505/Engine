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
	// �� ��Ʈ ���� �ʱ�ȭ
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
	// �������� ���� �ʱ�ȭ �ȵ������ÿ��� �׳� ��ȯ
	if (!m_Renderer)
		return;

	// ���� �ʺ� ���
	float width = static_cast<float>(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
	float height = static_cast<float>(ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);

	// �� ��Ʈ�� ���� �ʺ�� ���� ���� �ʺ� �ٸ��� ����
	if (m_Width != width || m_Height != height)
	{
		m_Renderer->SetViewport(width, height);

		m_Width = width;
		m_Height = height;
	}

	// �� ��Ʈ ������ ����(��ŷ ��Ȯ���� ���ؼ�)
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

	// ����ۿ� �������� ���� �̹����� �����ͼ� �׷�����.
	ImGuiEX::Image(m_Renderer->GetFrameTexture(), ImVec2(static_cast<float>(m_Width), static_cast<float>(m_Height)));

	// ���� ���콺�� �� ��Ʈ �ȿ� �����ϴ���
	m_Input->SetMouseIsInViewport(ImGui::IsItemHovered());

	// Ŭ�� ���� Ŭ���� ��ŷ�� ��ƼƼ�� ��ȯ�Ѵ�.
	if (ImGui::IsMouseClicked(0) && ImGui::IsItemHovered())
		EditorHelper::Get().PickEntity();

	if (auto payload = ImGuiEX::ReceiveDragPayload(ImGuiEX::DragPayloadType::DragPayload_Model))
		EditorHelper::Get().LoadModel(get<const char*>(payload->data));
}