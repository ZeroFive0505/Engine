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

// 월드 뷰어에서 사용될 전역 변수
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
	// 월드 뷰어 위젯 초기화
	m_Title = "World";
	m_Flags |= ImGuiWindowFlags_HorizontalScrollbar;

	Widget_World::g_World = m_Context->GetSubModule<World>();
	Widget_World::g_Input = m_Context->GetSubModule<Input>();

	// 엔티티 선택시 콜백 함수
	EditorHelper::Get().g_OnEntity_selected = [this]() {SetSelectedEntity(EditorHelper::Get().g_SelectedEntity.lock(), false); };
}

void WorldViewer::UpdateVisible()
{
	if (Widget_World::g_World->IsLoading())
		return;

	// 트리 렌더링
	TreeShow();

	// 마우스 왼쪽 버튼 릴리즈가 됬으며 선택된 엔티티가 있을시
	if (ImGui::IsMouseReleased(0) && Widget_World::g_EntityClicked)
	{
		// 현재 호버링된 엔티티와 선택된 엔티티가 같다면
		if (Widget_World::g_EntityHovered && Widget_World::g_EntityHovered->GetObjectID() == Widget_World::g_EntityClicked->GetObjectID())
		{
			// 엔티티 선택
			SetSelectedEntity(Widget_World::g_EntityClicked->GetSharedPtr());
		}

		// 초기화
		Widget_World::g_EntityClicked = nullptr;
	}
}

void WorldViewer::TreeShow()
{
	// 트리 시작
	OnTreeBegin();

	// 루트부터 시작 기본으로 열린 상태
	if (ImGui::TreeNodeEx("Root", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth))
	{
		// 페이로드 리시브
		if (auto payload = ImGuiEX::ReceiveDragPayload(ImGuiEX::DragPayloadType::DragPayload_Entity))
		{
			// 엔티티의 아이디를 가져온다.
			const uint64_t entity_id = get<uint64_t>(payload->data);

			// 월드에서 아이디를 기준으로 드랍된 엔티티를 찾아온다.
			if (const shared_ptr<Entity>& dropped_entity = Widget_World::g_World->EntityGetByID(entity_id))
			{
				// 루트에서 드랍됬을경우 부모를 삭제
				dropped_entity->GetTransform()->SetParent(nullptr);
			}
		}

		// 루트 엔티티부터 시작해서 모든 엔티티를 가져온다.
		vector<shared_ptr<Entity>> root_entites = Widget_World::g_World->EntityGetRoots();
		for (const shared_ptr<Entity>& entity : root_entites)
		{
			TreeAddEntity(entity.get());
		}

		// 트리 오픈시 자동 스크롤
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

	// 계층구조에서 보이지 않으면 추가는 불가능하다.
	if (!is_visible_in_hierarchy)
		return;

	// 모든 자식들을 가져온다.
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

	// 보이는 자식이 존재할경우 플래그를 추가
	// 만약 존재하지 않으면 리프노드
	node_flags |= has_visible_children ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf;

	if (const shared_ptr<Entity> selected_entity = EditorHelper::Get().g_SelectedEntity.lock())
	{
		// 현재 선택된 엔티티가 추가된 엔티티와 같다면 선택
		node_flags |= selected_entity->GetObjectID() == entity->GetObjectID() ? ImGuiTreeNodeFlags_Selected : node_flags;

		// 선택된 엔티티까지 자동 스크롤
		if (m_Expand_to_selection)
		{
			// 자식인지 확인
			if (selected_entity->GetTransform()->IsDescendantOf(entity->GetTransform()))
			{
				// 자식이라면 자동으로 노드를 열고 스크롤한다.
				ImGui::SetNextItemOpen(true);
				m_Expanded_to_selection = true;
			}
		}
	}

	// 노드 아이디
	const void* node_id = reinterpret_cast<void*>(static_cast<uint64_t>(entity->GetObjectID()));
	// 노드 이름
	string node_name = entity->GetObjectName();
	// 노드가 오픈되어 있는지 확인
	const bool is_node_open = ImGui::TreeNodeEx(node_id, node_flags, node_name.c_str());

	// 노드가 선택되었고 자동 스크롤이라면
	if ((node_flags & ImGuiTreeNodeFlags_Selected) && m_Expand_to_selection)
	{
		m_Selected_entity_rect = ImGui::GetCurrentContext()->LastItemData.Rect;
	}

	// 호버링된 위젯 설정
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
		Widget_World::g_EntityHovered = entity;

	// 드래그 드랍 관리
	EntityHandleDragDrop(entity);

	if (is_node_open)
	{
		if (has_visible_children)
		{
			// 보이는 자식 엔티티는 추가한다.
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
	// 호버링중인지 확인
	const bool is_window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	const bool left_click = ImGui::IsMouseClicked(0);
	const bool right_click = ImGui::IsMouseClicked(1);

	// 호버링중이 아니라면 반환
	if (!is_window_hovered)
		return;

	// 왼쪽 클릭시에는 엔티티 선택
	if (left_click && Widget_World::g_EntityHovered)
	{
		Widget_World::g_EntityClicked = Widget_World::g_EntityHovered;
	}

	// 오른쪽 클릭시에는
	if (right_click)
	{
		// 호버링되 위젯이 존재할시에
		if (Widget_World::g_EntityHovered)
		{
			// 엔티티 선택
			SetSelectedEntity(Widget_World::g_EntityHovered->GetSharedPtr());
		}

		// 그리고 팝업을 띄운다.
		ImGui::OpenPopup("##HierarchyContextMenu");
	}

	// 엔티티 선택
	if ((left_click || right_click) && !Widget_World::g_EntityHovered)
	{
		SetSelectedEntity(m_EmptyEntity);
	}
}

void WorldViewer::EntityHandleDragDrop(Entity* entity_ptr) const
{
	// 드래그, 드랍 설정

	// 드래그 드랍 시작
	if (ImGui::BeginDragDropSource())
	{
		// 페이로드를 설정한다.
		// 데이터는 아이디
		Widget_World::g_Payload.data = entity_ptr->GetObjectID();
		// 타입은 엔티티
		Widget_World::g_Payload.type = ImGuiEX::DragPayloadType::DragPayload_Entity;
		// 페이로드 생성
		ImGuiEX::CreateDragPayload(Widget_World::g_Payload);
		ImGui::EndDragDropSource();
	}

	// 페이로드 리시브
	if (auto payload = ImGuiEX::ReceiveDragPayload(ImGuiEX::DragPayloadType::DragPayload_Entity))
	{
		// 아이디를 가져온다.
		const uint64_t entity_id = get<uint64_t>(payload->data);

		if (const shared_ptr<Entity>& dropped_entity = Widget_World::g_World->EntityGetByID(entity_id))
		{
			// 만약 드랍된 페이로드의 아이디가 현재 엔티티의 아이디와 다르다면
			// 부모 설정
			if (dropped_entity->GetObjectID() != entity_ptr->GetObjectID())
			{
				dropped_entity->GetTransform()->SetParent(entity_ptr->GetTransform());
			}
		}
	}
}

void WorldViewer::SetSelectedEntity(const shared_ptr<Entity>& entity, const bool from_editor /*= true*/)
{
	// 엔티티 선택
	m_Expand_to_selection = true;

	if (from_editor)
	{
		EditorHelper::Get().SetSelectedEntity(entity);
	}

	// 엔티티 프로퍼티
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

	// 엔티티 컨텍스트 메뉴 출력

	const auto selected_entity = EditorHelper::Get().g_SelectedEntity.lock();
	const bool on_entity = selected_entity != nullptr;

	// 선택된 엔티티가 존재할시에는 복사
	if (on_entity)
	{
		if (ImGui::MenuItem("Copy"))
		{
			Widget_World::g_EntityCopied = selected_entity.get();
		}
	}

	// 엔티티 새롭게 클론
	if (ImGui::MenuItem("Paste"))
	{
		if (Widget_World::g_EntityCopied)
		{
			Widget_World::g_EntityCopied->Clone();
		}
	}

	// 엔티티를 오른쪽 클릭시에는
	if (on_entity)
	{
		// 리네임
		if (ImGui::MenuItem("Rename"))
		{
			Widget_World::g_PopUpRenameEntity = true;
		}
	}

	if (on_entity)
	{
		// 삭제
		if (ImGui::MenuItem("Delete", "Delete"))
		{
			ActionEntityDelete(selected_entity);
		}
	}

	ImGui::Separator();

	// 빈곳에서 오른쪽 클릭시

	// 빈 엔티티 생성
	if (ImGui::MenuItem("Create Empty"))
	{
		ActionEntityCreateEmpty();
	}

	// 3D 오브젝트 생성
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

	// 카메라
	if (ImGui::MenuItem("Camera"))
	{
		ActionEntityCreateCamera();
	}

	// 라이트
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

	// 스카이박스
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
	// 엔티티 리네임 팝업

	if (Widget_World::g_PopUpRenameEntity)
	{
		ImGui::OpenPopup("##RenameEntity");
		Widget_World::g_PopUpRenameEntity = false;
	}

	if (ImGui::BeginPopup("##RenameEntity"))
	{
		// 현재 선택된 엔티티를 가져온다.
		auto selected_entity = EditorHelper::Get().g_SelectedEntity.lock();

		// 만약 존재하지 않는다면 팝업을 닫고 끝낸다.
		if (!selected_entity)
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		// 이름
		string name = selected_entity->GetObjectName();

		ImGui::Text("Name");
		// 텍스트 입력창을 띄운다.
		ImGui::InputText("##Edit", &name);
		// 이름 재설정
		selected_entity->SetName(name);

		// 버튼 클릭시 팝업을 닫는다.
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

	// 저장: Ctrl + S
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

	// 로드: Ctrl + L
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
