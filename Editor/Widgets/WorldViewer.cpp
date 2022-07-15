#include "WorldViewer.h"
#include "Properties.h"
#include "MenuBar.h"
#include "../Editor.h"
#include "ImGuiHelper.h"
#include "../ImGui/Source/imgui_stdlib.h"
#include "Rendering/Model.h"
#include "World/Entity.h"
#include "World/Components/Transform.h"
#include "World/Components/Light.h"
#include "World/Components/RigidBody.h"
#include "World/Components/Collider.h"
#include "World/Components/Constraint.h"
#include "World/Components/Environment.h"
#include "World/Components/Renderable.h"

using namespace std;
using namespace PlayGround;

// ���� ���� ���� ���� ����
namespace Widget_World
{
	static PlayGround::World* g_World = nullptr;
	static Input* g_Input = nullptr;
	static bool g_PopUpRenameEntity = false;
	static ImGuiEX::sDragDropPayload g_Payload;

	static Entity* g_EntityCopied = nullptr;
	static Entity* g_EntityHovered = nullptr;
	static Entity* g_EntityClicked = nullptr;
}

WorldViewer::WorldViewer(Editor* editor) : Widget(editor)
{
	// ���� ��� ���� �ʱ�ȭ
	m_Title = "World";
	m_Flags |= ImGuiWindowFlags_HorizontalScrollbar;

	Widget_World::g_World = m_Context->GetSubModule<World>();
	Widget_World::g_Input = m_Context->GetSubModule<Input>();

	// ��ƼƼ ���ý� �ݹ� �Լ�
	EditorHelper::Get().g_OnEntity_selected = [this]() {SetSelectedEntity(EditorHelper::Get().g_SelectedEntity.lock(), false); };
}

void WorldViewer::UpdateVisible()
{
	if (Widget_World::g_World->IsLoading())
		return;

	// Ʈ�� ������
	TreeShow();

	// ���콺 ���� ��ư ����� ������ ���õ� ��ƼƼ�� ������
	if (ImGui::IsMouseReleased(0) && Widget_World::g_EntityClicked)
	{
		// ���� ȣ������ ��ƼƼ�� ���õ� ��ƼƼ�� ���ٸ�
		if (Widget_World::g_EntityHovered && Widget_World::g_EntityHovered->GetObjectID() == Widget_World::g_EntityClicked->GetObjectID())
		{
			// ��ƼƼ ����
			SetSelectedEntity(Widget_World::g_EntityClicked->GetSharedPtr());
		}

		// �ʱ�ȭ
		Widget_World::g_EntityClicked = nullptr;
	}
}

void WorldViewer::TreeShow()
{
	// Ʈ�� ����
	OnTreeBegin();

	// ��Ʈ���� ���� �⺻���� ���� ����
	if (ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth))
	{
		// ���̷ε� ���ú�
		if (auto payload = ImGuiEX::ReceiveDragPayload(ImGuiEX::DragPayloadType::DragPayload_Entity))
		{
			// ��ƼƼ�� ���̵� �����´�.
			const uint64_t entity_id = get<uint64_t>(payload->data);

			// ���忡�� ���̵� �������� ����� ��ƼƼ�� ã�ƿ´�.
			if (const shared_ptr<Entity>& dropped_entity = Widget_World::g_World->EntityGetByID(entity_id))
			{
				// ��Ʈ���� ���������� �θ� ����
				dropped_entity->GetTransform()->SetParent(nullptr);
			}
		}

		// ��Ʈ ��ƼƼ���� �����ؼ� ��� ��ƼƼ�� �����´�.
		vector<shared_ptr<Entity>> root_entites = Widget_World::g_World->EntityGetRoots();
		for (const shared_ptr<Entity>& entity : root_entites)
		{
			TreeAddEntity(entity.get());
		}

		// Ʈ�� ���½� �ڵ� ��ũ��
		if (m_Expand_to_selection && !m_Expanded_to_selection)
		{
			ImGui::ScrollToBringRectIntoView(m_Window, m_Selected_entity_rect);
			m_Expand_to_selection = false;
		}

		ImGui::TreePop();
	}

	OnTreeEnd();
}

void WorldViewer::OnTreeBegin()
{
	Widget_World::g_EntityHovered = nullptr;
}

void WorldViewer::OnTreeEnd()
{
	HandleKeyShortcuts();
	HandleClicking();
	PopUps();
}

void WorldViewer::TreeAddEntity(Entity* entity)
{
	if (!entity)
		return;

	m_Expanded_to_selection = false;
	bool is_selected_entity = false;
	const bool is_visible_in_hierarchy = entity->IsVisibleInHierarchy();
	bool has_visible_children = false;

	// ������������ ������ ������ �߰��� �Ұ����ϴ�.
	if (!is_visible_in_hierarchy)
		return;

	// ��� �ڽĵ��� �����´�.
	const vector<Transform*>& children = entity->GetTransform()->GetChildren();

	for (Transform* child : children)
	{
		if (child->GetEntity()->IsVisibleInHierarchy())
		{
			has_visible_children = true;
			break;
		}
	}

	ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanFullWidth;

	// ���̴� �ڽ��� �����Ұ�� �÷��׸� �߰�
	// ���� �������� ������ �������
	node_flags |= has_visible_children ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf;

	if (const shared_ptr<Entity> selected_entity = EditorHelper::Get().g_SelectedEntity.lock())
	{
		// ���� ���õ� ��ƼƼ�� �߰��� ��ƼƼ�� ���ٸ� ����
		node_flags |= selected_entity->GetObjectID() == entity->GetObjectID() ? ImGuiTreeNodeFlags_Selected : node_flags;

		// ���õ� ��ƼƼ���� �ڵ� ��ũ��
		if (m_Expand_to_selection)
		{
			// �ڽ����� Ȯ��
			if (selected_entity->GetTransform()->IsDescendantOf(entity->GetTransform()))
			{
				// �ڽ��̶�� �ڵ����� ��带 ���� ��ũ���Ѵ�.
				ImGui::SetNextItemOpen(true);
				m_Expanded_to_selection = true;
			}
		}
	}

	// ��� ���̵�
	const void* node_id = reinterpret_cast<void*>(static_cast<uint64_t>(entity->GetObjectID()));
	// ��� �̸�
	string node_name = entity->GetObjectName();
	// ��尡 ���µǾ� �ִ��� Ȯ��
	const bool is_node_open = ImGui::TreeNodeEx(node_id, node_flags, node_name.c_str());

	// ��尡 ���õǾ��� �ڵ� ��ũ���̶��
	if ((node_flags & ImGuiTreeNodeFlags_Selected) && m_Expand_to_selection)
	{
		m_Selected_entity_rect = ImGui::GetCurrentContext()->LastItemData.Rect;
	}

	// ȣ������ ���� ����
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
		Widget_World::g_EntityHovered = entity;

	// �巡�� ��� ����
	EntityHandleDragDrop(entity);

	if (is_node_open)
	{
		if (has_visible_children)
		{
			// ���̴� �ڽ� ��ƼƼ�� �߰��Ѵ�.
			for (const auto& child : children)
			{
				if (!child->GetEntity()->IsVisibleInHierarchy())
					continue;

				TreeAddEntity(child->GetEntity());
			}
		}

		ImGui::TreePop();
	}
}

void WorldViewer::HandleClicking()
{
	// ȣ���������� Ȯ��
	const bool is_window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	const bool left_click = ImGui::IsMouseClicked(0);
	const bool right_click = ImGui::IsMouseClicked(1);

	// ȣ�������� �ƴ϶�� ��ȯ
	if (!is_window_hovered)
		return;

	// ���� Ŭ���ÿ��� ��ƼƼ ����
	if (left_click && Widget_World::g_EntityHovered)
	{
		Widget_World::g_EntityClicked = Widget_World::g_EntityHovered;
	}

	// ������ Ŭ���ÿ���
	if (right_click)
	{
		// ȣ������ ������ �����ҽÿ�
		if (Widget_World::g_EntityHovered)
		{
			// ��ƼƼ ����
			SetSelectedEntity(Widget_World::g_EntityHovered->GetSharedPtr());
		}

		// �׸��� �˾��� ����.
		ImGui::OpenPopup("##HierarchyContextMenu");
	}

	// ��ƼƼ ����
	if ((left_click || right_click) && !Widget_World::g_EntityHovered)
	{
		SetSelectedEntity(m_EmptyEntity);
	}
}

void WorldViewer::EntityHandleDragDrop(Entity* entity_ptr) const
{
	// �巡��, ��� ����

	// �巡�� ��� ����
	if (ImGui::BeginDragDropSource())
	{
		// ���̷ε带 �����Ѵ�.
		// �����ʹ� ���̵�
		Widget_World::g_Payload.data = entity_ptr->GetObjectID();
		// Ÿ���� ��ƼƼ
		Widget_World::g_Payload.type = ImGuiEX::DragPayloadType::DragPayload_Entity;
		// ���̷ε� ����
		ImGuiEX::CreateDragPayload(Widget_World::g_Payload);
		ImGui::EndDragDropSource();
	}

	// ���̷ε� ���ú�
	if (auto payload = ImGuiEX::ReceiveDragPayload(ImGuiEX::DragPayloadType::DragPayload_Entity))
	{
		// ���̵� �����´�.
		const uint64_t entity_id = get<uint64_t>(payload->data);

		if (const shared_ptr<Entity>& dropped_entity = Widget_World::g_World->EntityGetByID(entity_id))
		{
			// ���� ����� ���̷ε��� ���̵� ���� ��ƼƼ�� ���̵�� �ٸ��ٸ�
			// �θ� ����
			if (dropped_entity->GetObjectID() != entity_ptr->GetObjectID())
			{
				dropped_entity->GetTransform()->SetParent(entity_ptr->GetTransform());
			}
		}
	}
}

void WorldViewer::SetSelectedEntity(const shared_ptr<Entity>& entity, const bool from_editor /*= true*/)
{
	// ��ƼƼ ����
	m_Expand_to_selection = true;

	if (from_editor)
	{
		EditorHelper::Get().SetSelectedEntity(entity);
	}

	// ��ƼƼ ������Ƽ
	Properties::Inspect(entity);
}

void WorldViewer::PopUps()
{
	PopUpContextMenu();
	PopUpEntityRename();
}

void WorldViewer::PopUpContextMenu() const
{
	if (!ImGui::BeginPopup("##HierarchyContextMenu"))
		return;

	// ��ƼƼ ���ؽ�Ʈ �޴� ���

	const auto selected_entity = EditorHelper::Get().g_SelectedEntity.lock();
	const bool on_entity = selected_entity != nullptr;

	// ���õ� ��ƼƼ�� �����ҽÿ��� ����
	if (on_entity)
	{
		if (ImGui::MenuItem("Copy"))
		{
			Widget_World::g_EntityCopied = selected_entity.get();
		}
	}

	// ��ƼƼ ���Ӱ� Ŭ��
	if (ImGui::MenuItem("Paste"))
	{
		if (Widget_World::g_EntityCopied)
		{
			Widget_World::g_EntityCopied->Clone();
		}
	}

	// ��ƼƼ�� ������ Ŭ���ÿ���
	if (on_entity)
	{
		// ������
		if (ImGui::MenuItem("Rename"))
		{
			Widget_World::g_PopUpRenameEntity = true;
		}
	}

	if (on_entity)
	{
		// ����
		if (ImGui::MenuItem("Delete", "Delete"))
		{
			ActionEntityDelete(selected_entity);
		}
	}

	ImGui::Separator();

	// ������� ������ Ŭ����

	// �� ��ƼƼ ����
	if (ImGui::MenuItem("Create Empty"))
	{
		ActionEntityCreateEmpty();
	}

	// 3D ������Ʈ ����
	if (ImGui::BeginMenu("3D Objects"))
	{
		if (ImGui::MenuItem("Cube"))
		{
			ActionEntityCreateCube();
		}
		else if (ImGui::MenuItem("Quad"))
		{
			ActionEntityCreateQuad();
		}
		else if (ImGui::MenuItem("Sphere"))
		{
			ActionEntityCreateSphere();
		}
		else if (ImGui::MenuItem("Cylinder"))
		{
			ActionEntityCreateCylinder();
		}
		else if (ImGui::MenuItem("Cone"))
		{
			ActionEntityCreateCone();
		}

		ImGui::EndMenu();
	}

	// ī�޶�
	if (ImGui::MenuItem("Camera"))
	{
		ActionEntityCreateCamera();
	}

	// ����Ʈ
	if (ImGui::BeginMenu("Light"))
	{
		if (ImGui::MenuItem("Directional"))
		{
			ActionEntityCreateLightDirectional();
		}
		else if (ImGui::MenuItem("Point"))
		{
			ActionEntityCreateLightPoint();
		}
		else if (ImGui::MenuItem("Spot"))
		{
			ActionEntityCreateLightSpot();
		}

		ImGui::EndMenu();
	}

	// ��ī�̹ڽ�
	if (ImGui::BeginMenu("Environment"))
	{
		if (ImGui::MenuItem("Environment"))
		{
			ActionEntityCreateSkybox();
		}

		ImGui::EndMenu();
	}

	ImGui::EndPopup();
}

void WorldViewer::PopUpEntityRename() const
{
	// ��ƼƼ ������ �˾�

	if (Widget_World::g_PopUpRenameEntity)
	{
		ImGui::OpenPopup("##RenameEntity");
		Widget_World::g_PopUpRenameEntity = false;
	}

	if (ImGui::BeginPopup("##RenameEntity"))
	{
		// ���� ���õ� ��ƼƼ�� �����´�.
		auto selected_entity = EditorHelper::Get().g_SelectedEntity.lock();

		// ���� �������� �ʴ´ٸ� �˾��� �ݰ� ������.
		if (!selected_entity)
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		// �̸�
		string name = selected_entity->GetObjectName();

		ImGui::Text("Name");
		// �ؽ�Ʈ �Է�â�� ����.
		ImGui::InputText("##Edit", &name);
		// �̸� �缳��
		selected_entity->SetName(name);

		// ��ư Ŭ���� �˾��� �ݴ´�.
		if (ImGuiEX::Button("OK"))
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		ImGui::EndPopup();
	}
}

void WorldViewer::HandleKeyShortcuts()
{
	if (Widget_World::g_Input->GetKey(EKeyCode::Delete))
	{
		ActionEntityDelete(EditorHelper::Get().g_SelectedEntity.lock());
	}

	// ����: Ctrl + S
	if (Widget_World::g_Input->GetKey(EKeyCode::Ctrl_Left) && Widget_World::g_Input->GetKeyDown(EKeyCode::S))
	{
		const string& file_path = Widget_World::g_World->GetFilePath();

		if (file_path.empty())
		{
			m_Editor->GetWidget<MenuBar>()->ShowWorldSaveDialog();
		}
		else
		{
			EditorHelper::Get().SaveWorld(Widget_World::g_World->GetFilePath());
		}
	}

	// �ε�: Ctrl + L
	if (Widget_World::g_Input->GetKey(EKeyCode::Ctrl_Left) && Widget_World::g_Input->GetKeyDown(EKeyCode::L))
	{
		m_Editor->GetWidget<MenuBar>()->ShowWorldLoadDialog();
	}
}

void WorldViewer::ActionEntityDelete(const shared_ptr<Entity>& entity)
{
	Widget_World::g_World->EntityRemove(entity);
}

Entity* WorldViewer::ActionEntityCreateEmpty()
{
	shared_ptr<Entity> entity = Widget_World::g_World->EntityCreate();

	if (const shared_ptr<Entity> selected_entity = EditorHelper::Get().g_SelectedEntity.lock())
	{
		entity->GetTransform()->SetParent(selected_entity->GetTransform());
	}

	return entity.get();
}

void WorldViewer::ActionEntityCreateCube()
{
	auto entity = ActionEntityCreateEmpty();
	auto renderable = entity->AddComponent<Renderable>();
	renderable->GeometrySet(Geometry_Default_Cube);
	renderable->UseDefaultMaterial();
	entity->SetName("Cube");
}

void WorldViewer::ActionEntityCreateQuad()
{
	auto entity = ActionEntityCreateEmpty();
	auto renderable = entity->AddComponent<Renderable>();
	renderable->GeometrySet(Geometry_Default_Quad);
	renderable->UseDefaultMaterial();
	entity->SetName("Quad");
}

void WorldViewer::ActionEntityCreateSphere()
{
	auto entity = ActionEntityCreateEmpty();
	auto renderable = entity->AddComponent<Renderable>();
	renderable->GeometrySet(Geometry_Default_Sphere);
	renderable->UseDefaultMaterial();
	entity->SetName("Sphere");
}

void WorldViewer::ActionEntityCreateCylinder()
{
	auto entity = ActionEntityCreateEmpty();
	auto renderable = entity->AddComponent<Renderable>();
	renderable->GeometrySet(Geometry_Default_Cylinder);
	renderable->UseDefaultMaterial();
	entity->SetName("Cylinder");
}

void WorldViewer::ActionEntityCreateCone()
{
	auto entity = ActionEntityCreateEmpty();
	auto renderable = entity->AddComponent<Renderable>();
	renderable->GeometrySet(Geometry_Default_Cone);
	renderable->UseDefaultMaterial();
	entity->SetName("Cone");
}

void WorldViewer::ActionEntityCreateCamera()
{
	auto entity = ActionEntityCreateEmpty();
	entity->AddComponent<Camera>();
	entity->SetName("Camera");
}

void WorldViewer::ActionEntityCreateLightDirectional()
{
	auto entity = ActionEntityCreateEmpty();
	entity->AddComponent<Light>()->SetLightType(LightType::Directional);
	entity->SetName("Directional");
}

void WorldViewer::ActionEntityCreateLightPoint()
{
	auto entity = ActionEntityCreateEmpty();
	entity->SetName("Point");

	Light* light = entity->AddComponent<Light>();
	light->SetLightType(LightType::Point);
	light->SetIntensity(2600.0f);
}

void WorldViewer::ActionEntityCreateLightSpot()
{
	auto entity = ActionEntityCreateEmpty();
	entity->SetName("Spot");

	Light* light = entity->AddComponent<Light>();
	light->SetLightType(LightType::Spot);
	light->SetIntensity(2600.0f);
}

void WorldViewer::ActionEntityCreateSkybox()
{
	auto entity = ActionEntityCreateEmpty();
	entity->AddComponent<Environment>();
	entity->SetName("Environment");
}
