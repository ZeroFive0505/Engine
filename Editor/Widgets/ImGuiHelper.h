#pragma once

#include <string>
#include <variant>
#include <chrono>
#include "../ImGui/Source/imgui.h"
#include "../ImGui/Source/imgui_internal.h"
#include "RHI/RHI_Texture.h"
#include "RHI/RHI_Texture2D.h"
#include "Rendering/Renderer.h"
#include "Resource/ResourceCache.h"
#include "Threading/Threading.h"
#include "Input/Input.h"
#include "World/World.h"
#include "World/Components/Camera.h"
#include "World/TransformHandle/TransformHandle.h"
#include "Display/Display.h"
#include "../IconProvider.h"


// 에디터 헬퍼 클래스
class EditorHelper
{
public:

	static EditorHelper& Get()
	{
		static EditorHelper instance;
		return instance;
	}

	void Initialize(PlayGround::Context* context)
	{
		// 에디터 헬퍼 초기화
		// 엔진 주요 기능을 가져온다.
		g_Context = context;
		g_ResourceCache = context->GetSubModule<PlayGround::ResourceCache>();
		g_World = context->GetSubModule<PlayGround::World>();
		g_Threading = context->GetSubModule<PlayGround::Threading>();
		g_Renderer = context->GetSubModule<PlayGround::Renderer>();
		g_Input = context->GetSubModule<PlayGround::Input>();
	}

	void LoadModel(const std::string& file_path) const
	{
		// 모델 로딩
		auto resource_cache = g_ResourceCache;

		// 리소스 캐시에서 모델을 로딩을 스레드로 넘긴다.
		g_Threading->AddTask([resource_cache, file_path]()
		{
			resource_cache->Load<PlayGround::Model>(file_path);
		});
	}

	void LoadWorld(const std::string& file_path) const
	{
		// 월드 로딩

		auto world = g_World;

		// 스레드를 먼저 비운다.
		g_Threading->Flush(true);

		// 스레드에 월드 로딩 태스크를 추가한다.
		g_Threading->AddTask([world, file_path]()
		{
			world->LoadFromFile(file_path);
		});
	}

	void SaveWorld(const std::string& file_path) const
	{
		// 월드 저장
		auto world = g_World;

		// 월드 저장 태스크 추가
		g_Threading->AddTask([world, file_path]()
		{
			world->SaveToFile(file_path);
		});
	}

	// 엔티티 피킹
	void PickEntity()
	{
		// 만약 트랜스폼 핸들이 현재 사용되고 있다면 그냥 반환
		if (g_World->GetTransformHandle()->IsEditing())
			return;

		// 렌더러의 카메라를 가져온다.
		const auto& camera = g_Renderer->GetCamera();

		// 카메라가 없다면 그냥 반환
		if (!camera)
			return;

		// 피킹 시작
		std::shared_ptr<PlayGround::Entity> entity;
		camera->Pick(entity);

		// 피킹 설정
		SetSelectedEntity(entity);

		// 피킹 콜백
		g_OnEntity_selected();
	}

	void SetSelectedEntity(const std::shared_ptr<PlayGround::Entity>& entity)
	{
		// 피킹이 성공했으므로 트랜스폼 핸들도 피킹된 오브젝트로 이동시킨다.
		g_SelectedEntity = g_World->GetTransformHandle()->SetSelectedEntity(entity);
	}

	PlayGround::Context* g_Context = nullptr;
	PlayGround::ResourceCache* g_ResourceCache = nullptr;
	PlayGround::World* g_World = nullptr;
	PlayGround::Threading* g_Threading = nullptr;
	PlayGround::Renderer* g_Renderer = nullptr;
	PlayGround::Input* g_Input = nullptr;
	std::weak_ptr<PlayGround::Entity> g_SelectedEntity;
	std::function<void()> g_OnEntity_selected = nullptr;
};

// ImGui 편의를 위한 네임스페이스
namespace ImGuiEX
{
	// 기본 틴트 색깔
	static const ImVec4 default_tint(255, 255, 255, 255);

	// 윈도우 컨텐트 너비 계산
	inline float GetWindowContentRegionWidth()
	{
		return ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
	}

	// 헤더 간소화
	inline bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		bool result = ImGui::CollapsingHeader(label, flags);
		ImGui::PopStyleVar();
		return result;
	}

	// 버튼 생성
	inline bool Button(const char* label, const PlayGround::Math::Vector2& size = PlayGround::Math::Vector2::Zero)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		bool result = ImGui::Button(label, size);
		ImGui::PopStyleVar();
		return result;
	}

	// 이미지 버튼 생성 (텍스쳐가 들어왔을때)
	inline bool ImageButton(PlayGround::RHI_Texture* texture, const ImVec2& size)
	{
		return ImGui::ImageButton(
			static_cast<ImTextureID>(texture),
			size,
			ImVec2(0.0f, 0.0f),
			ImVec2(1.0f, 1.0f),
			ImColor(0.0f, 0.0f, 0.0f, 0.0f),
			default_tint
		);
	}

	// 아이콘 타입으로 버튼 생성
	inline bool ImageButton(const EIconType icon, const float size, bool border = false)
	{
		if (!border)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		}

		bool result = ImGui::ImageButton(
			static_cast<ImTextureID>(IconProvider::Get().GetTextureByType(icon)),
			ImVec2(size, size),
			ImVec2(0, 0),           // uv0
			ImVec2(1, 1),           // uv1
			-1,                     // frame padding
			ImColor(0, 0, 0, 0),    // background
			default_tint            // tint
		);

		if (!border)
		{
			ImGui::PopStyleVar();
		}

		return result;
	}

	// 아이디로 이미지와 아이콘으로 이미지 버튼 생성
	inline bool ImageButton(const char* id, const EIconType icon, const float size, bool border = false)
	{
		if (!border)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		}

		ImGui::PushID(id);
		const auto pressed = ImGui::ImageButton(
			static_cast<ImTextureID>(IconProvider::Get().GetTextureByType(icon)),
			ImVec2(size, size),
			ImVec2(0, 0),           // uv0
			ImVec2(1, 1),           // uv1
			-1,                     // frame padding
			ImColor(0, 0, 0, 0),    // background
			default_tint            // tint
		);
		ImGui::PopID();

		if (!border)
		{
			ImGui::PopStyleVar();
		}

		return pressed;
	}

	// 썸네일을 통해 이미지 생성
	inline void Image(const sThumbnail& thumbnail, const float size)
	{
		ImGui::Image(
			static_cast<ImTextureID>(IconProvider::Get().GetTextureByThumbnail(thumbnail)),
			ImVec2(size, size),
			ImVec2(0, 0),
			ImVec2(1, 1),
			default_tint,       // tint
			ImColor(0, 0, 0, 0) // border
		);
	}

	// 텍스쳐를 통해 이미지 생성
	inline void Image(PlayGround::RHI_Texture* texture, const PlayGround::Math::Vector2& size, bool border = false)
	{
		if (!border)
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

		ImGui::Image(
			static_cast<ImTextureID>(texture),
			size,
			ImVec2(0.0f, 0.0f),
			ImVec2(1.0f, 1.0f),
			default_tint,
			ImColor(0.0f, 0.0f, 0.0f, 0.0f)
		);

		if (!border)
			ImGui::PopStyleVar();
	}

	inline void Image(PlayGround::RHI_Texture* texture, const ImVec2& size, 
		const ImColor& tint = default_tint, const ImColor& border = ImColor(0, 0, 0, 0))
	{
		ImGui::Image(
			static_cast<ImTextureID>(texture),
			size,
			ImVec2(0, 0),
			ImVec2(1, 1),
			tint,
			border
		);
	}

	inline void Image(const EIconType icon, const float size)
	{
		ImGui::Image(
			static_cast<void*>(IconProvider::Get().GetTextureByType(icon)),
			ImVec2(size, size),
			ImVec2(0, 0),
			ImVec2(1, 1),
			default_tint,       // tint
			ImColor(0, 0, 0, 0) // border
		);
	}

	// 페이로드 타입
	enum class DragPayloadType
	{
		DragPayload_Unknown,
		DragPayload_Texture,
		DragPayload_Entity,
		DragPayload_Model,
		DragPayload_Audio,
		DragPayload_Material
	};

	// 페이로드 구조체
	struct sDragDropPayload
	{
		typedef std::variant<const char*, uint64_t> dataVariant;
		sDragDropPayload(const DragPayloadType _type = DragPayloadType::DragPayload_Unknown, const dataVariant _data = nullptr)
		{
			type = _type;
			data = _data;
		}

		DragPayloadType type;
		dataVariant data;
	};

	// 페이로드 생성
	inline void CreateDragPayload(const sDragDropPayload& payload)
	{
		ImGui::SetDragDropPayload(reinterpret_cast<const char*>(&payload.type),
			reinterpret_cast<const void*>(&payload), sizeof(payload), ImGuiCond_Once);
	}

	// 페이로드 리시브
	inline sDragDropPayload* ReceiveDragPayload(DragPayloadType type)
	{
		// 페이로드 드랍 시작
		if (ImGui::BeginDragDropTarget())
		{
			if (const auto payload_imgui = ImGui::AcceptDragDropPayload(reinterpret_cast<const char*>(&type)))
			{
				return static_cast<sDragDropPayload*>(payload_imgui->Data);
			}

			ImGui::EndDragDropTarget();
		}

		return nullptr;
	}

	// 이미지 슬롯
	inline void ImageSlot(const std::shared_ptr<PlayGround::RHI_Texture>& image, const std::function<void(const std::shared_ptr<PlayGround::RHI_Texture>&)>& setter)
	{
		const ImVec2 slot_size = ImVec2(80.0f, 80.0f);
		const float button_size = 15.0f;

		ImGui::BeginGroup();
		{
			PlayGround::RHI_Texture* texture = image.get();
			const ImVec2 pos_image = ImGui::GetCursorPos();
			const ImVec2 pos_button = ImVec2(ImGui::GetCursorPosX() + slot_size.x - button_size * 2.0f + 6.0f, ImGui::GetCursorPosY() + 1.0f);

			if (image != nullptr)
			{
				ImGui::SetCursorPos(pos_button);
				ImGui::PushID(static_cast<int>(pos_button.x + pos_button.y));

				if (ImGuiEX::ImageButton("", EIconType::Component_Material_RemoveTexture, button_size, true))
				{
					texture = nullptr;
					setter(nullptr);
				}
				ImGui::PopID();
			}

			ImGui::SetCursorPos(pos_image);
			ImGuiEX::Image(
				texture,
				slot_size,
				ImColor(255, 255, 255, 255),
				ImColor(255, 255, 255, 128)
			);

			if (texture != nullptr)
			{
				ImGui::SetCursorPos(pos_button);
				ImGuiEX::ImageButton("", EIconType::Component_Material_RemoveTexture, button_size, true);
			}
		}

		ImGui::EndGroup();

		if (auto payload = ImGuiEX::ReceiveDragPayload(ImGuiEX::DragPayloadType::DragPayload_Texture))
		{
			try
			{
				if (const auto tex = EditorHelper::Get().g_ResourceCache->Load<PlayGround::RHI_Texture2D>(std::get<const char*>(payload->data)))
				{
					setter(tex);
				}
			}
			catch (const std::bad_variant_access& e)
			{
				LOG_ERROR("%s", e.what());
			}
		}
	}

	// 툴팁 생성
	inline void ToolTip(const char* text)
	{
		if (!text)
			return;

		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text(text);
			ImGui::EndTooltip();
		}
	}

	// 부동소수점 입력기
	inline void DragFloatWrap(const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", const ImGuiSliderFlags flags = 0)
	{
		ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, flags);

		if (ImGui::IsItemEdited() && ImGui::IsMouseDown(0))
		{
			PlayGround::Math::Vector2 pos = EditorHelper::Get().g_Input->GetMousePosition();
			uint32_t edge_padding = 5;

			bool wrapped = false;

			if (pos.x >= PlayGround::Display::GetWidth() - edge_padding)
			{
				pos.x = static_cast<float>(edge_padding + 1);
				wrapped = true;
			}
			else if (pos.x <= edge_padding)
			{
				pos.x = static_cast<float>(PlayGround::Display::GetWidth() - edge_padding - 1);
				wrapped = true;
			}

			if (wrapped)
			{
				ImGuiIO imgui_io = ImGui::GetIO();
				imgui_io.MousePos = pos;
				imgui_io.MousePosPrev = pos;
				imgui_io.WantSetMousePos = true;
			}
		}
	}

	// 콤보 박스
	inline bool ComboBox(const char* label, const std::vector<std::string>& options, uint32_t* selection_index)
	{
		const uint32_t option_count = static_cast<uint32_t>(options.size());

		if (*selection_index >= option_count)
		{
			*selection_index = option_count - 1;
		}

		bool selection_mode = false;

		std::string selection_string = options[*selection_index];

		if (ImGui::BeginCombo(label, selection_string.c_str()))
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(options.size()); i++)
			{
				const bool is_selected = *selection_index == i;

				if (ImGui::Selectable(options[i].c_str(), is_selected))
				{
					*selection_index = i;
					selection_mode = true;
				}

				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		return selection_mode;
	}
}