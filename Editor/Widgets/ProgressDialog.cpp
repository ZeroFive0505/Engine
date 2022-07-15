#include "ProgressDialog.h"
#include "Resource/ProgressTracker.h"

using namespace std;
using namespace PlayGround;
using namespace PlayGround::Math;

ProgressDialog::ProgressDialog(Editor* editor) : Widget(editor)
{
	// ���̾�α� Ÿ��Ʋ�� �ʱⰪ
	m_Title = "Please wait...";
	m_Visible = false;
	m_Progress = 0.0f;
	m_InitSize = Vector2(500.0f, 83.0f);
	m_Flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking;
	m_Position = widget_position_screen_center;
}

void ProgressDialog::UpdateAlways()
{
	// Ʈ��Ŀ�� �����´�.
	ProgressTracker& progress_report = ProgressTracker::Get();
	const bool is_loading_model = progress_report.GetIsLoading(EProgressType::ModelImporter);
	const bool is_loading_scene = progress_report.GetIsLoading(EProgressType::World);
	const bool in_progress = is_loading_model || is_loading_scene;

	// �ε� ���� ����
	if (is_loading_model)
	{
		m_Progress = progress_report.GetPercentage(EProgressType::ModelImporter);
		m_ProgressStatus = progress_report.GetStatus(EProgressType::ModelImporter);
	}
	else if (is_loading_scene)
	{
		m_Progress = progress_report.GetPercentage(EProgressType::World);
		m_ProgressStatus = progress_report.GetStatus(EProgressType::World);
	}

	SetVisible(in_progress);
}

void ProgressDialog::UpdateVisible()
{
	ImGui::SetWindowFocus();
	ImGui::PushItemWidth(m_InitSize.x - ImGui::GetStyle().WindowPadding.x * 2.0f);
	ImGui::ProgressBar(m_Progress, ImVec2(0.0f, 0.0f));
	ImGui::Text(m_ProgressStatus.c_str());
	ImGui::PopItemWidth();
}