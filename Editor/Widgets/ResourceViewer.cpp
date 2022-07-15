#include "ResourceViewer.h"
#include "Resource/ResourceCache.h"
#include "../ImGui/Source/imgui.h"
#include "Core/EngineObject.h"
#include "Math/Vector2.h"

using namespace std;
using namespace PlayGround;
using namespace PlayGround::Math;

// 사용중인 메모리 출력
void PrintMemory(uint64_t memory)
{
	if (memory == 0)
		ImGui::Text("0 Mb");
	else if (memory < 1024)
		ImGui::Text("%.4f Mb", static_cast<float>(memory) / 1000.0f / 1000.0f);
	else
		ImGui::Text("%.1f Mb", static_cast<float>(memory) / 1000.0f / 1000.0f);
}

// 리소스 뷰어
ResourceViewer::ResourceViewer(Editor* editor) : Widget(editor)
{
	m_Title = "Resource Viewer";
	m_InitSize = Vector2(1366.0f, 768.0f);
	m_Visible = false;
	m_Position = widget_position_screen_center;
}

void ResourceViewer::UpdateVisible()
{
	auto resource_cache = m_Context->GetSubModule<ResourceCache>();
	auto resources = resource_cache->GetByType();
	const float memory_usage_cpu = resource_cache->GetMemoryUsageCPU() / 1000.0f / 1000.0f;
	const float memory_usage_gpu = resource_cache->GetMemoryUsageGPU() / 1000.0f / 1000.0f;

	// 현재 사용중인 리소스 수, 메모리 사용량 출력
	ImGui::Text("Resource count: %d, Memory usage cpu: %d Mb, Memory usage gpu: %d Mb", static_cast<uint32_t>(resources.size()), static_cast<uint32_t>(memory_usage_cpu), static_cast<uint32_t>(memory_usage_gpu));
	ImGui::Separator();

	static ImGuiTableFlags flags =
		ImGuiTableFlags_Borders |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable | 
		ImGuiTableFlags_ContextMenuInBody |
		ImGuiTableFlags_ScrollX | 
		ImGuiTableFlags_ScrollY;             

	static ImVec2 size = ImVec2(-1.0f);

	// 테이블 시작
	if (ImGui::BeginTable("##Widget_ResourceCache", 7, flags, size))
	{
		ImGui::TableSetupColumn("Type");
		ImGui::TableSetupColumn("ID");
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Path");
		ImGui::TableSetupColumn("Path (native)");
		ImGui::TableSetupColumn("Size CPU");
		ImGui::TableSetupColumn("Size GPU");
		ImGui::TableHeadersRow();

		for (const shared_ptr<IResource>& resource : resources)
		{
			if (EngineObject* object = dynamic_cast<EngineObject*>(resource.get()))
			{
				// 행 변경
				ImGui::TableNextRow();

				// 타입
				ImGui::TableSetColumnIndex(0);
				ImGui::Text(resource->GetResourceTypeCStr());

				// 아이디
				ImGui::TableSetColumnIndex(1);
				ImGui::Text(to_string(object->GetObjectID()).c_str());

				// 이름
				ImGui::TableSetColumnIndex(2);
				ImGui::Text(resource->GetResourceName().c_str());

				// 경로
				ImGui::TableSetColumnIndex(3);
				ImGui::Text(resource->GetResourceFilePath().c_str());

				// 자체 경로
				ImGui::TableSetColumnIndex(4);
				ImGui::Text(resource->GetResourceFilePathNative().c_str());

				// CPU 메모리 사용량
				ImGui::TableSetColumnIndex(5);
				PrintMemory(object->GetObjectSizeCPU());

				// GPU 메모리 사용량
				ImGui::TableSetColumnIndex(6);
				PrintMemory(object->GetObjectSizeGPU());
			}
		}

		ImGui::EndTable();
	}
}