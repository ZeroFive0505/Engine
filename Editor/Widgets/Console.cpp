#include "Console.h"
#include "Rendering/Model.h"
#include "ImGuiHelper.h"
#include "../ImGui/Source/imgui_internal.h"
#include "../IconProvider.h"

using namespace std;
using namespace PlayGround;
using namespace Math;

Console::Console(Editor* editor) : Widget(editor)
{
	// ������ ������ �ܼ�
	m_Title = "Console";

	// �ΰ� ����
	m_Logger = make_shared<EngineLogger>();
	// �ΰ��� �ݹ� �Լ� ����
	m_Logger->SetCallback([this](const sLogPackage& package) {
		AddLogPackage(package);
	});

	// �ΰ� ����
	Logger::SetLogger(m_Logger);
}

void Console::UpdateVisible()
{
	// Ŭ���� ��ư
	if (ImGuiEX::Button("Clear"))
		Clear(); ImGui::SameLine();

	// �α� Ÿ�� ���
	const auto button_log_type_visibility_toggle = [this](const EIconType icon, uint32_t index)
	{
		// ���� �ε����� �α� ���ü� ���θ� �����´�.
		bool& visibility = m_LogType_visibility[index];
		// ��Ÿ�� Ǫ��
		ImGui::PushStyleColor(ImGuiCol_Button, visibility ? ImGui::GetStyle().Colors[ImGuiCol_Button] : ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);

		// �̹��� ��ư Ŭ���� ���ü� ���
		if (ImGuiEX::ImageButton(icon, 15.0f))
			visibility = !visibility;

		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::Text("%d", m_LogType_counts[index]);
		ImGui::SameLine();
	};

	// ��ư
	button_log_type_visibility_toggle(EIconType::Console_Info, 0);
	button_log_type_visibility_toggle(EIconType::Console_Warning, 1);
	button_log_type_visibility_toggle(EIconType::Console_Error, 2);

	// ��
	const float label_width = 37.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
	// ����
	m_LogFilter.Draw("Filter", ImGui::GetContentRegionAvail().x - label_width);
	ImGui::PopStyleVar();
	ImGui::Separator();

	// ���� �ٸ� �����尡 ���� �۾����̶�� ��ٸ���.
	while (m_IsReading)
	{
		this_thread::sleep_for(std::chrono::microseconds(16));
	}

	// �۾� ����
	m_IsReading = true;

	// �÷���
	static const ImGuiTableFlags table_flags = 
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_ScrollX |
		ImGuiTableFlags_ScrollY;

	static const ImVec2 size = ImVec2(-1.0f);

	// ���̺� ����
	if (ImGui::BeginTable("##Widget_console_content", 1, table_flags, size))
	{
		// �α��� ����ŭ ���� �����.
		for (uint32_t row = 0; row < m_Logs.size(); row++)
		{
			sLogPackage& log = m_Logs[row];
			
			// ���͸�
			if (m_LogFilter.PassFilter(log.text.c_str()) && m_LogType_visibility[log.error_level])
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				ImGui::PushID(row);
				{
					{
						ImGui::PushStyleColor(ImGuiCol_Text, m_LogTypeColor[log.error_level]);
						ImGui::TextUnformatted(log.text.c_str());
						ImGui::PopStyleColor(1);
					}

					// �˾� ����
					if (ImGui::BeginPopupContextItem("##Widget_console_contextmenu"))
					{
						// �α� ī��
						if (ImGui::MenuItem("Copy"))
						{
							ImGui::LogToClipboard();
							ImGui::LogText("%s", log.text.c_str());
							ImGui::LogFinish();
						}

						ImGui::Separator();

						// ���ͳ����� Ž��
						if (ImGui::MenuItem("Search"))
						{
							FileSystem::OpenDirectoryWindow("https://www.google.com/search?q=" + log.text);
						}

						ImGui::EndPopup();
					}
				}

				ImGui::PopID();
			}
		}

		// �� �Ʒ��� ��ũ��
		if (m_ScrollToBottom)
		{
			ImGui::SetScrollHereY();
			m_ScrollToBottom = false;
		}

		ImGui::EndTable();
	}

	// �۾� ��
	m_IsReading = false;
}

void Console::AddLogPackage(const sLogPackage& package)
{
	// ���������� �ٸ� �����尡 �а� �������� �α׸� �߰������ʴ´�.
	while (m_IsReading)
	{
		this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	// �� �α� �߰�
	m_Logs.push_back(package);

	// ���� ���ġ�� �Ѿ�ٸ� ���� ������ �α� ����
	if (static_cast<uint32_t>(m_Logs.size()) > m_LogMaxCount)
		m_Logs.pop_front();

	// �α� ī��Ʈ ����
	m_LogType_counts[package.error_level]++;

	if (m_LogType_visibility[package.error_level])
		m_ScrollToBottom = true;
}

void Console::Clear()
{
	m_Logs.clear();
	m_Logs.shrink_to_fit();

	m_LogType_counts[0] = 0;
	m_LogType_counts[1] = 0;
	m_LogType_counts[2] = 0;
}