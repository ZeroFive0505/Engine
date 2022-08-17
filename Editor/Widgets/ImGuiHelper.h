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


// ������ ���� Ŭ����
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
		// ������ ���� �ʱ�ȭ
		// ���� �ֿ� ����� �����´�.
		g_Context = context;
		g_ResourceCache = context->GetSubModule<PlayGround::ResourceCache>();
		g_World = context->GetSubModule<PlayGround::World>();
		g_Threading = context->GetSubModule<PlayGround::Threading>();
		g_Renderer = context->GetSubModule<PlayGround::Renderer>();
		g_Input = context->GetSubModule<PlayGround::Input>();
	}

	void LoadModel(const std::string& file_path) const
	{
		// �� �ε�
		auto resource_cache = g_ResourceCache;

		// ���ҽ� ĳ�ÿ��� ���� �ε��� ������� �ѱ��.
		g_Threading->AddTask([resource_cache, file_path]()
		{
			resource_cache->Load<PlayGround::Model>(file_path);
		});
	}

	void LoadWorld(const std::string& file_path) const
	{
		// ���� �ε�

		auto world = g_World;

		// �����带 ���� ����.
		g_Threading->Flush(true);

		// �����忡 ���� �ε� �½�ũ�� �߰��Ѵ�.
		g_Threading->AddTask([world, file_path]()
		{
			world->LoadFromFile(file_path);
		});
	}

	void SaveWorld(const std::string& file_path) const
	{
		// ���� ����
		auto world = g_World;

		// ���� ���� �½�ũ �߰�
		g_Threading->AddTask([world, file_path]()
		{
			world->SaveToFile(file_path);
		});
	}

	// ��ƼƼ ��ŷ
	void PickEntity()
	{
		// ���� Ʈ������ �ڵ��� ���� ���ǰ� �ִٸ� �׳� ��ȯ
		if (g_World->GetTransformHandle()->IsEditing())
			return;

		// �������� ī�޶� �����´�.
		const auto& camera = g_Renderer->GetCamera();

		// ī�޶� ���ٸ� �׳� ��ȯ
		if (!camera)
			return;

		// ��ŷ ����
		std::shared_ptr<PlayGround::Entity> entity;
		camera->Pick(entity);

		// ��ŷ ����
		SetSelectedEntity(entity);

		// ��ŷ �ݹ�
		g_OnEntity_selected();
	}

	void SetSelectedEntity(const std::shared_ptr<PlayGround::Entity>& entity)
	{
		// ��ŷ�� ���������Ƿ� Ʈ������ �ڵ鵵 ��ŷ�� ������Ʈ�� �̵���Ų��.
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

// ImGui ���Ǹ� ���� ���ӽ����̽�
namespace ImGuiEX
{
	// �⺻ ƾƮ ����
	static const ImVec4 default_tint(255, 255, 255, 255);

	// ������ ����Ʈ �ʺ� ���
	inline float GetWindowContentRegionWidth()
	{
		return ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
	}

	// ��� ����ȭ
	inline bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		bool result = ImGui::CollapsingHeader(label, flags);
		ImGui::PopStyleVar();
		return result;
	}

	// ��ư ����
	inline bool Button(const char* label, const PlayGround::Math::Vector2& size = PlayGround::Math::Vector2::Zero)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		bool result = ImGui::Button(label, size);
		ImGui::PopStyleVar();
		return result;
	}

	// �̹��� ��ư ���� (�ؽ��İ� ��������)
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

	// ������ Ÿ������ ��ư ����
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

	// ���̵�� �̹����� ���������� �̹��� ��ư ����
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

	// ������� ���� �̹��� ����
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

	// �ؽ��ĸ� ���� �̹��� ����
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

	// ���̷ε� Ÿ��
	enum class DragPayloadType
	{
		DragPayload_Unknown,
		DragPayload_Texture,
		DragPayload_Entity,
		DragPayload_Model,
		DragPayload_Audio,
		DragPayload_Material
	};

	// ���̷ε� ����ü
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

	// ���̷ε� ����
	inline void CreateDragPayload(const sDragDropPayload& payload)
	{
		ImGui::SetDragDropPayload(reinterpret_cast<const char*>(&payload.type),
			reinterpret_cast<const void*>(&payload), sizeof(payload), ImGuiCond_Once);
	}

	// ���̷ε� ���ú�
	inline sDragDropPayload* ReceiveDragPayload(DragPayloadType type)
	{
		// ���̷ε� ��� ����
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

	// �̹��� ����
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

	// ���� ����
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

	// �ε��Ҽ��� �Է±�
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

	// �޺� �ڽ�
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