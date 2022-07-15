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
	// 위젯의 제목은 콘솔
	m_Title = "Console";

	// 로거 생성
	m_Logger = make_shared<EngineLogger>();
	// 로거의 콜백 함수 설정
	m_Logger->SetCallback([this](const sLogPackage& package) {
		AddLogPackage(package);
	});

	// 로거 설정
	Logger::SetLogger(m_Logger);
}

void Console::UpdateVisible()
{
	// 클리어 버튼
	if (ImGuiEX::Button("Clear"))
		Clear(); ImGui::SameLine();

	// 로그 타입 토글
	const auto button_log_type_visibility_toggle = [this](const EIconType icon, uint32_t index)
	{
		// 현재 인덱스의 로그 가시성 여부를 가져온다.
		bool& visibility = m_LogType_visibility[index];
		// 스타일 푸시
		ImGui::PushStyleColor(ImGuiCol_Button, visibility ? ImGui::GetStyle().Colors[ImGuiCol_Button] : ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);

		// 이미지 버튼 클릭시 가시성 토글
		if (ImGuiEX::ImageButton(icon, 15.0f))
			visibility = !visibility;

		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::Text("%d", m_LogType_counts[index]);
		ImGui::SameLine();
	};

	// 버튼
	button_log_type_visibility_toggle(EIconType::Console_Info, 0);
	button_log_type_visibility_toggle(EIconType::Console_Warning, 1);
	button_log_type_visibility_toggle(EIconType::Console_Error, 2);

	// 라벨
	const float label_width = 37.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
	// 필터
	m_LogFilter.Draw("Filter", ImGui::GetContentRegionAvail().x - label_width);
	ImGui::PopStyleVar();
	ImGui::Separator();

	// 만약 다른 스레드가 먼저 작업중이라면 기다린다.
	while (m_IsReading)
	{
		this_thread::sleep_for(std::chrono::microseconds(16));
	}

	// 작업 시작
	m_IsReading = true;

	// 플래그
	static const ImGuiTableFlags table_flags = 
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_BordersOuter |
		ImGuiTableFlags_ScrollX |
		ImGuiTableFlags_ScrollY;

	static const ImVec2 size = ImVec2(-1.0f);

	// 테이블 시작
	if (ImGui::BeginTable("##Widget_console_content", 1, table_flags, size))
	{
		// 로그의 수만큼 행을 만든다.
		for (uint32_t row = 0; row < m_Logs.size(); row++)
		{
			sLogPackage& log = m_Logs[row];
			
			// 필터링
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

					// 팝업 시작
					if (ImGui::BeginPopupContextItem("##Widget_console_contextmenu"))
					{
						// 로그 카피
						if (ImGui::MenuItem("Copy"))
						{
							ImGui::LogToClipboard();
							ImGui::LogText("%s", log.text.c_str());
							ImGui::LogFinish();
						}

						ImGui::Separator();

						// 인터넷으로 탐색
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

		// 맨 아래로 스크롤
		if (m_ScrollToBottom)
		{
			ImGui::SetScrollHereY();
			m_ScrollToBottom = false;
		}

		ImGui::EndTable();
	}

	// 작업 끝
	m_IsReading = false;
}

void Console::AddLogPackage(const sLogPackage& package)
{
	// 마찬가지로 다른 스레드가 읽고 있을때는 로그를 추가하지않는다.
	while (m_IsReading)
	{
		this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	// 새 로그 추가
	m_Logs.push_back(package);

	// 만약 허용치가 넘어갔다면 제일 오래된 로그 제거
	if (static_cast<uint32_t>(m_Logs.size()) > m_LogMaxCount)
		m_Logs.pop_front();

	// 로그 카운트 증가
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