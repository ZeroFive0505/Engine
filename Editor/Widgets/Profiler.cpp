#include "Profiler.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Core/Context.h"
#include "ImGuiHelper.h"

using namespace std;
using namespace PlayGround::Math;

// �������Ϸ����� ������ Ÿ�� ���
static void ShowTimeBlock(const PlayGround::TimeBlock& time_block, float total_time)
{
	float tree_depth_stride = 10.0f;

	// Ÿ�Ӻ���� �̸�
	const char* name = time_block.GetName();
	// Ÿ�Ӻ���� �Ⱓ
	const float duration = time_block.GetDuration();
	// ����
	const float fraction = duration / total_time;
	// Ÿ�Ӻ�� �ʺ�
	const float width = fraction * ImGuiEX::GetWindowContentRegionWidth();
	const ImVec4& color = ImGui::GetStyle().Colors[ImGuiCol_CheckMark];
	const ImVec2 pos_screen = ImGui::GetCursorScreenPos();
	const ImVec2 pos = ImGui::GetCursorPos();
	const float text_height = ImGui::CalcTextSize(name, nullptr, true).y;

	// ������ Ŀ���� Ŀ������ �ʺ� Ŀ����.
	ImGui::GetWindowDrawList()->AddRectFilled(pos_screen, ImVec2(pos_screen.x + width, pos_screen.y + text_height), IM_COL32(color.x * 255, color.y * 255, color.z * 255, 255));
	ImGui::SetCursorPos(ImVec2(pos.x + tree_depth_stride * time_block.GetTreeDepth(), pos.y));
	ImGui::Text("%s - %.2f ms", name, duration);
}

Profiler::Profiler(Editor* editor) : Widget(editor)
{
	// �������Ϸ� �ʱ�ȭ
	m_Flags |= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar;
	m_Title = "Profiler";
	m_Visible = false;
	m_Profiler = m_Context->GetSubModule<PlayGround::Profiler>();
	m_InitSize = Vector2(1000.0f, 715.0f);
	m_Position = widget_position_screen_center;
}

void Profiler::OnShow()
{
	m_Profiler->SetEnabled(true);
}

void Profiler::OnHide()
{
	m_Profiler->SetEnabled(false);
}

void Profiler::UpdateVisible()
{
	int prev_item_type = m_ItemType;

	ImGui::RadioButton("CPU", &m_ItemType, 0);
	ImGui::SameLine();

	ImGui::RadioButton("GPU", &m_ItemType, 1);
	ImGui::SameLine();

	// �������Ϸ� ���� ���͹�
	float interval = m_Profiler->GetUpdateInterval();
	ImGui::DragFloat("Update interval (The smaller the interval the higher the performance impact)", &interval, 0.001f, 0.0f, 0.5f);
	m_Profiler->SetUpdateInverval(interval);
	ImGui::Separator();

	// Ÿ�Ӻ�� Ÿ��
	PlayGround::TimeBlockType type = m_ItemType == 0 ? PlayGround::TimeBlockType::CPU : PlayGround::TimeBlockType::GPU;
	const std::vector<PlayGround::TimeBlock>& time_blocks = m_Profiler->GetTimeBlocks();
	const uint32_t time_block_count = static_cast<uint32_t>(time_blocks.size());
	float time_last = type == PlayGround::TimeBlockType::CPU ? m_Profiler->GetTimeCPULast() : m_Profiler->GetTimeGPULast();

	// Ÿ�Ӻ�� ������ŭ �ݺ��Ѵ�.
	for (uint32_t i = 0; i < time_block_count; i++)
	{
		// Ÿ���� ���� �ʴٸ� ����
		if (time_blocks[i].GetType() != type)
			continue;

		// ���� i��° Ÿ�Ӻ���� ������ �ʾҴٸ� ��ȯ
		if (!time_blocks[i].IsComplete())
			return;

		ShowTimeBlock(time_blocks[i], time_last);
	}

	ImGui::Separator();
	{
		if (prev_item_type != m_ItemType)
		{
			m_Plot.fill(0.0f);
			m_Timings.Clear();
		}

		if (time_last == 0.0f)
		{
			time_last = m_Plot.back();
		}
		else
		{
			m_Timings.AddSample(time_last);
		}

		{
			if (ImGui::Button("Clear"))
				m_Timings.Clear();

			ImGui::SameLine();
			ImGui::Text("Current: %.2f, Avg: %.2f, Min: %.2f, Max: %.2f", time_last, m_Timings.m_Avg, m_Timings.m_Min, m_Timings.m_Max);
			bool is_stuttering = type == PlayGround::TimeBlockType::CPU ? m_Profiler->IsCPUStuttering() : m_Profiler->IsGPUStuttering();
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(is_stuttering ? 1.0f : 0.0f, is_stuttering ? 0.0f : 1.0f, 0.0f, 1.0f), is_stuttering ? "Stuttering: Yes" : "Stuttering: No");
		}

		for (uint32_t i = 0; i < m_Plot.size() - 1; i++)
		{
			m_Plot[i] = m_Plot[i + 1];
		}

		m_Plot[m_Plot.size() - 1] = time_last;

		ImGui::PlotLines("", m_Plot.data(), static_cast<int>(m_Plot.size()), 0, "", 
			m_Timings.m_Min, m_Timings.m_Max, ImVec2(ImGuiEX::GetWindowContentRegionWidth(), 80));
	}

	if (type == PlayGround::TimeBlockType::GPU)
	{
		ImGui::Separator();

		const uint32_t memory_used = m_Profiler->GetGpuMemoryUsed();
		const uint32_t memory_available = m_Profiler->GetGpuMemoryAvailable();
		const string overlay = "Memory " + to_string(memory_used) + "/" + to_string(memory_available) + " MB";

		ImGui::ProgressBar((float)memory_used / (float)memory_available, ImVec2(-1, 0), overlay.c_str());
	}
}