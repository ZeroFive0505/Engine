#include "Properties.h"
#include "ImGuiHelper.h"
#include "../ImGui/Source/imgui_stdlib.h"
#include "../ImGui/Source/imgui_internal.h"
#include "ColorPicker.h"
#include "Core/Engine.h"
#include "Rendering/Model.h"
#include "World/Entity.h"
#include "World/Components/Transform.h"
#include "World/Components/Renderable.h"
#include "World/Components/RigidBody.h"
#include "World/Components/Collider.h"
#include "World/Components/Light.h"
#include "World/Components/Environment.h"
#include "../IconProvider.h"

using namespace std;
using namespace PlayGround;
using namespace Math;

// 현재 선택된 엔티티, 마테리얼
weak_ptr<Entity> Properties::m_InspectedEntity;
weak_ptr<Material> Properties::m_InspectedMaterial;

// 프로퍼티 헬퍼 네임 스페이스
namespace Helper
{
	static ResourceCache* resource_cache;
	static World* world;
	static Vector3 rotation_hint;

	static string g_ContextMenuID;
	static float g_Column = 180.0f;
	static const float g_MaxWidth = 100.0f;
	// 복사된 컴포넌트
	static IComponent* g_Copied;

	// 컴포넌트 옵션
	inline void ComponentContextMenu_Options(const string& id, IComponent* component, const bool removable)
	{
		// 팝업 시작
		if (ImGui::BeginPopup(id.c_str()))
		{
			// 삭제가 가능하다면
			if (removable)
			{
				// 삭제 메뉴
				if (ImGui::MenuItem("Remove"))
				{
					if (auto entity = Properties::m_InspectedEntity.lock())
					{
						if (component)
						{
							entity->RemoveComponentByID(component->GetObjectID());
						}
					}
				}
			}

			// 복사
			if (ImGui::MenuItem("Copy Attributes"))
			{
				g_Copied = component;
			}

			if (ImGui::MenuItem("Paste Attributes"))
			{
				// 만약 복사된 컴포넌트가 존재하고 복사 대상이 같은 컴포넌트 타입이라면
				if (g_Copied && g_Copied->GetComponentType() == component->GetComponentType())
				{
					// 값을 복사해간다.
					component->SetAttributes(g_Copied->GetAttributes());
				}
			}

			ImGui::EndPopup();
		}
	}

	// 컴포넌트 시작
	inline bool ComponentBegin(const string& name, const EIconType icon_enum, IComponent* component_instance, bool options = true, const bool removable = true)
	{
		const bool collapsed = ImGuiEX::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_DefaultOpen);

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// 컴포넌트 옵션
		if (options)
		{
			// 아이콘 너비
			const float icon_width = 16.0f;
			// y 위치
			const float original_pen_y = ImGui::GetCursorPosY();

			// y 위치 설정
			ImGui::SetCursorPosY(original_pen_y + 5.0f);
			// 이미지 출력
			ImGuiEX::Image(icon_enum, 15);
			ImGui::SameLine(ImGuiEX::GetWindowContentRegionWidth() - icon_width + 1.0f); ImGui::SetCursorPosY(original_pen_y);
			// 같은 라인에 이미지 버튼 추가
			if (ImGuiEX::ImageButton(name.c_str(), EIconType::Component_Options, icon_width))
			{
				g_ContextMenuID = name;
				// 팝업 시작
				ImGui::OpenPopup(g_ContextMenuID.c_str());
			}

			if (g_ContextMenuID == name)
			{
				ComponentContextMenu_Options(g_ContextMenuID, component_instance, removable);
			}
		}

		return collapsed;
	}

	inline bool ComponentBegin(const string& name, const string& label_name, IComponent* component_instance, bool options = true, const bool removable = true)
	{
		const bool collapsed = ImGuiEX::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_DefaultOpen);

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		if (options)
		{
			const float icon_width = 16.0f;
			const float original_pen_y = ImGui::GetCursorPosY();

			ImGui::SetCursorPosY(original_pen_y + 5.0f);
			ImGui::Text(label_name.c_str());
			ImGui::SameLine(ImGuiEX::GetWindowContentRegionWidth() - icon_width + 1.0f); ImGui::SetCursorPosY(original_pen_y);
			if (ImGuiEX::ImageButton(name.c_str(), EIconType::Component_Options, icon_width))
			{
				g_ContextMenuID = name;
				ImGui::OpenPopup(g_ContextMenuID.c_str());
			}

			if (g_ContextMenuID == name)
			{
				ComponentContextMenu_Options(g_ContextMenuID, component_instance, removable);
			}
		}

		return collapsed;
	}

	inline void ComponentEnd()
	{
		ImGui::Separator();
	}
}

Properties::Properties(Editor* editor) : Widget(editor)
{
	// 프로퍼티 위젯 초기화
	m_Title = "Properties";
	m_InitSize.x = 500;

	m_LightColorPicker = make_unique<ColorPicker>("Light Color Picker");
	m_MaterialColorPicker = make_unique<ColorPicker>("Material Color Picker");
	m_CameraColorPicker = make_unique<ColorPicker>("Camera Color Picker");

	Helper::resource_cache = m_Context->GetSubModule<ResourceCache>();
	Helper::world = m_Context->GetSubModule<World>();
}

void Properties::UpdateVisible()
{
	// 만약 월드 로딩이 끝나지 않았다면 그냥 반환한다.
	if (m_Context->GetSubModule<World>()->IsLoading())
		return;

	// 위젯 너비
	ImGui::PushItemWidth(Helper::g_MaxWidth);

	// 만약 현재 선택된 엔티티가 존재한다면
	if (!m_InspectedEntity.expired())
	{
		// 엔티티 컴포넌트를 가져온다.
		shared_ptr<Entity> entity_ptr = m_InspectedEntity.lock();
		Renderable* renderable = entity_ptr->GetComponent<Renderable>();
		Material* material = renderable ? renderable->GetMaterial() : nullptr;

		// 엔티티가 가지고있는 컴포넌트 모두 출력
		ShowTransform(entity_ptr->GetComponent<Transform>());
		ShowLight(entity_ptr->GetComponent<Light>());
		ShowCamera(entity_ptr->GetComponent<Camera>());
		ShowEnvironment(entity_ptr->GetComponent<Environment>());
		ShowRenderable(renderable);
		ShowMaterial(material);
		ShowRigidBody(entity_ptr->GetComponent<RigidBody>());
		ShowCollider(entity_ptr->GetComponent<Collider>());

		// 컴포넌트 추가버튼
		ShowAddComponentButton();
	}
	// 만약 마테리얼의 경우 마테리얼 관련 출력
	else if (!m_InspectedMaterial.expired())
	{
		ShowMaterial(m_InspectedMaterial.lock().get());
	}

	ImGui::PopItemWidth();
}

void Properties::Inspect(const weak_ptr<Entity>& entity)
{
	m_InspectedEntity = entity;

	// 현재 선택된 엔티티가 있다면 회전 값을 출력
	if (const auto shared_ptr = entity.lock())
	{
		Helper::rotation_hint = shared_ptr->GetTransform()->GetLocalRotation().ToEulerAngles();
	}
	else
	{
		Helper::rotation_hint = Vector3::Zero;
	}

	// 만약 마레티얼이 선택되어 있다면 마테리얼 저장
	if (!m_InspectedMaterial.expired())
	{
		m_InspectedMaterial.lock()->SaveToFile(m_InspectedMaterial.lock()->GetResourceFilePathNative());
	}

	m_InspectedMaterial.reset();
}

void Properties::Inspect(const weak_ptr<Material>& material)
{
	m_InspectedEntity.reset();
	m_InspectedMaterial = material;
}

void Properties::ShowTransform(Transform* transform) const
{
	// 트랜스폼

	enum class Axis
	{
		x,
		y,
		z
	};

	// 트랜스폼 컴포넌트 시작
	if (Helper::ComponentBegin("Transform", "Transform", transform, true, false))
	{
		// 플레이 여부 확인
		const bool is_playing = m_Context->m_Engine->IsEngineModeSet(GameMode);

		// 위치, 회전, 스케일 값
		Vector3 position = transform->GetLocalPosition();
		Vector3 rotation = transform->GetLocalRotation().ToEulerAngles();
		Vector3 scale = transform->GetLocalScale();

		// 부동소수점 값 렌더링 람다 함수
		const auto show_float = [](Axis axis, float* value)
		{
			// 스페이싱
			const float label_float_spacing = 15.0f;
			// 조정 수치는 0.01
			const float step = 0.01f;
			// 소수점 4번째 자리까지 표현
			const string format = "%.4f";

			// 축 라벨 출력
			ImGui::TextUnformatted(axis == Axis::x ? "x" : axis == Axis::y ? "y" : "z");
			// 스페이싱
			ImGui::SameLine(label_float_spacing);
			// 라벨 위치
			Vector2 pos_post_label = Vector2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);

			// 너비
			ImGui::PushItemWidth(128.0f);
			ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));
			// 소수점 입력 장치
			ImGuiEX::DragFloatWrap("##no_label", value, step, numeric_limits<float>::lowest(), numeric_limits<float>::max(), format.c_str());
			ImGui::PopID();
			ImGui::PopItemWidth();

			static const ImU32 color_x = IM_COL32(168, 46, 2, 255);
			static const ImU32 color_y = IM_COL32(112, 162, 22, 255);
			static const ImU32 color_z = IM_COL32(51, 122, 210, 255);
			static const Vector2 size = Vector2(4.0f, 19.0f);
			static const Vector2 offset = Vector2(5.0f, 4.0);
			pos_post_label += offset;
			ImRect axis_color_rect = ImRect(pos_post_label.x, pos_post_label.y, pos_post_label.x + size.x, pos_post_label.y + size.y);
			ImGui::GetWindowDrawList()->AddRectFilled(axis_color_rect.Min, axis_color_rect.Max, axis == Axis::x ? color_x : axis == Axis::y ? color_y : color_z);
		};

		// 위치, 회전, 스케일 세가지 값을 보여주는 람다 함수
		const auto show_vector = [&show_float](const char* label, Vector3& vector)
		{
			const float label_indetation = 15.0f;

			// 그룹 시작
			ImGui::BeginGroup();
			ImGui::Indent(label_indetation);
			ImGui::TextUnformatted(label);
			ImGui::Unindent(label_indetation);
			// 세가지 축 
			show_float(Axis::x, &vector.x);
			show_float(Axis::y, &vector.y);
			show_float(Axis::z, &vector.z);
			ImGui::EndGroup();
		};

		show_vector("Position", position);
		ImGui::SameLine();
		show_vector("Rotation", rotation);
		ImGui::SameLine();
		show_vector("Scale", scale);

		// 만약 플레이 모드가 아니라면
		if (!is_playing)
		{
			// 유저가 위치, 회전, 스케일 조정가능
			transform->SetLocalPosition(position);
			transform->SetLocalScale(scale);

			if (rotation != Helper::rotation_hint)
			{
				transform->SetLocalRotaion(Quaternion::FromEulerAngles(rotation));
				Helper::rotation_hint = rotation;
			}
		}
	}

	Helper::ComponentEnd();
}

void Properties::ShowLight(Light* light) const
{
	if (!light)
		return;

	// 라이트 컴포넌트 시작
	if (Helper::ComponentBegin("Light", "Light", light))
	{
		// 광원의 타입
		static vector<string> types = { "Directional", "Point", "Spot" };
		// 광원의 세기
		float intensity = light->GetIntensity();
		// 각도
		float angle = light->GetAngle() * Math::Util::RAD_TO_DEG * 2.0f;
		// 그림자 캐스팅 여부
		bool shadows = light->GetShadowsEnabled();
		// 스크린 그림자 여부
		bool shadows_screen_space = light->GetShadowsScreenSpaceEnabled();
		// 그림자 투명도 여부
		bool shadows_transparent = light->GetShadowsTransparentEnabled();
		// 볼류메트릭
		bool volumetric = light->GetVolumetricEnabled();
		// 바이어스
		float bias = light->GetBias();
		float normal_bias = light->GetNormalBias();
		float range = light->GetRange();
		// 광원 색깔 설정
		m_LightColorPicker->SetColor(light->GetColor());

		// 방향성 조명인지
		bool is_directional = light->GetLightType() == LightType::Directional;

		ImGui::Text("Type");
		ImGui::PushItemWidth(110.0f);
		ImGui::SameLine(Helper::g_Column);
		uint32_t selection_index = static_cast<uint32_t>(light->GetLightType());
		
		// 광원 타입 설정
		if (ImGuiEX::ComboBox("##LightType", types, &selection_index))
		{
			light->SetLightType(static_cast<LightType>(selection_index));
		}

		ImGui::PopItemWidth();

		// 광원 색깔
		ImGui::Text("Color");
		ImGui::SameLine(Helper::g_Column); m_LightColorPicker->Update();

		// 세기
		ImGui::Text(is_directional ? "Intensity (Lux)" : "Intensity (Lumens)");
		ImGui::SameLine(Helper::g_Column);
		float v_speed = is_directional ? 20.0f : 5.0f;
		float v_max = is_directional ? 128000.0f : 100000.0f;
		ImGui::PushItemWidth(300); ImGuiEX::DragFloatWrap("##LightIntensity", &intensity, v_speed, 0.0f, v_max); ImGui::PopItemWidth();

		// 그림자 여부
		ImGui::Text("Shadows");
		ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##LightShadows", &shadows);

		// 그림자 설정이 꺼저있을경우 비활성화
		ImGui::BeginDisabled(!shadows);
		{
			// 투명 그림자
			ImGui::Text("Transparent Shadows");
			ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##Light_shadows_transparent", &shadows_transparent);
			ImGuiEX::ToolTip("Allows transparent objects to cast colored translucent shadows");

			// 볼류메트릭 라이팅 옵션
			ImGui::Text("Volumetric");
			ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##Light_volumetric", &volumetric);
			ImGuiEX::ToolTip("The shadow map is used to determine which parts of the \"air\" should be lit");
		}

		ImGui::EndDisabled();

		// 스크린 스페이스 그림자
		ImGui::Text("Screen Space Shadows");
		ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##Light_shadows_screen_space", &shadows_screen_space);
		ImGuiEX::ToolTip("Small scale shadows which add detail were surfaces meet, also known as contact shadows");

		// 바이어스
		ImGui::Text("Bias");
		ImGui::SameLine(Helper::g_Column);
		ImGui::PushItemWidth(300); ImGui::InputFloat("##LightBias", &bias, 1.0f, 1.0f, "%.0f"); ImGui::PopItemWidth();

		ImGui::Text("Normal Bias");
		ImGui::SameLine(Helper::g_Column);
		ImGui::PushItemWidth(300); ImGui::InputFloat("##LightNormalBias", &normal_bias, 1.0f, 1.0f, "%.0f"); ImGui::PopItemWidth();

		// 방향성 조명일 경우
		if (light->GetLightType() != LightType::Directional)
		{
			ImGui::Text("Range");
			ImGui::SameLine(Helper::g_Column);
			ImGui::PushItemWidth(300); ImGuiEX::DragFloatWrap("##LightRange", &range, 0.01f, 0.0f, 1000.0f); ImGui::PopItemWidth();
		}

		// 스팟 라이트일 경우
		if (light->GetLightType() == LightType::Spot)
		{
			ImGui::Text("Angle");
			ImGui::SameLine(Helper::g_Column);
			ImGui::PushItemWidth(300); ImGuiEX::DragFloatWrap("##LightAngle", &angle, 0.01f, 1.0f, 179.0f); ImGui::PopItemWidth();
		}

		if (intensity != light->GetIntensity())                            light->SetIntensity(intensity);
		if (shadows != light->GetShadowsEnabled())                         light->SetShadowsEnabled(shadows);
		if (shadows_screen_space != light->GetShadowsScreenSpaceEnabled()) light->SetShadowsScreenSpaceEnabled(shadows_screen_space);
		if (shadows_transparent != light->GetShadowsTransparentEnabled())  light->SetShadowsTransparentEnabled(shadows_transparent);
		if (volumetric != light->GetVolumetricEnabled())                   light->SetVolumetricEnabled(volumetric);
		if (bias != light->GetBias())                                      light->SetBias(bias);
		if (normal_bias != light->GetNormalBias())                         light->SetNormalBias(normal_bias);
		if (angle != light->GetAngle() * Math::Util::RAD_TO_DEG * 0.5f)  light->SetAngle(angle * Math::Util::DEG_TO_RAD * 0.5f);
		if (range != light->GetRange())                                    light->SetRange(range);
		if (m_LightColorPicker->GetColor() != light->GetColor())          light->SetColor(m_LightColorPicker->GetColor());
	}

	Helper::ComponentEnd();
}

void Properties::ShowRenderable(PlayGround::Renderable* renderable) const
{
	if (!renderable)
		return;

	// 렌더러블 컴포넌트 시작
	if (Helper::ComponentBegin("Renderable", "Renderer", renderable))
	{
		// 메쉬 이름
		const string& mesh_name = renderable->GeometryName();
		// 마테리얼
		Material* material = renderable->GetMaterial();
		// 마테리얼 이름
		string material_name = material ? material->GetResourceName() : "N/A";
		// 그림자 생성 여부
		bool cast_shadows = renderable->GetCastShadows();

		// 메쉬
		ImGui::Text("Mesh");
		ImGui::SameLine(Helper::g_Column); ImGui::Text(mesh_name.c_str());

		// 마테리얼
		ImGui::Text("Material");
		ImGui::SameLine(Helper::g_Column);
		ImGui::PushID("##MaterialName");
		ImGui::PushItemWidth(200.0f);
		ImGui::InputText("", &material_name, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

		// 마테리얼 드래그 드랍 페이로드
		if (auto payload = ImGuiEX::ReceiveDragPayload(ImGuiEX::DragPayloadType::DragPayload_Material))
		{
			renderable->SetMaterial(std::get<const char*>(payload->data));
		}

		ImGui::PopItemWidth();
		ImGui::PopID();

		ImGui::Text("Cast Shadows");
		ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##RenderableCastShadows", &cast_shadows);

		if (cast_shadows != renderable->GetCastShadows())
			renderable->SetCastShadows(cast_shadows);
	}

	Helper::ComponentEnd();
}

void Properties::ShowRigidBody(PlayGround::RigidBody* rigid_body) const
{
	if (!rigid_body)
		return;

	// 리지드 바디 컴포넌트 시작
	if (Helper::ComponentBegin("RigidBody", "Rigid Body", rigid_body))
	{
		// 질량
		float mass = rigid_body->GetMass();
		// 마찰
		float friction = rigid_body->GetFriction();
		// 구름 마찰
		float friction_rolling = rigid_body->GetFrictionRolling();
		// 복원력
		float restitution = rigid_body->GetRestitution();
		// 중력 여부
		bool use_gravity = rigid_body->GetUseGravity();
		// 키네마틱 여부
		bool is_kinematic = rigid_body->GetIsKinematic();
		// 위치 고정
		bool freeze_pos_x = static_cast<bool>(rigid_body->GetPositionLock().x);
		bool freeze_pos_y = static_cast<bool>(rigid_body->GetPositionLock().y);
		bool freeze_pos_z = static_cast<bool>(rigid_body->GetPositionLock().z);
		
		// 회전 고정
		bool freeze_rot_x = static_cast<bool>(rigid_body->GetRotationLock().x);
		bool freeze_rot_y = static_cast<bool>(rigid_body->GetRotationLock().y);
		bool freeze_rot_z = static_cast<bool>(rigid_body->GetRotationLock().z);

		const auto input_text_flags = ImGuiInputTextFlags_CharsDecimal;
		const float item_width = 120.0f;
		const float step = 0.1f;
		const float step_fast = 0.1f;
		const char* precision = "%.3f";

		ImGui::Text("Mass");
		ImGui::SameLine(Helper::g_Column); ImGui::PushItemWidth(item_width); ImGui::InputFloat("##RigidBodyMass", &mass, step, step_fast, precision, input_text_flags); ImGui::PopItemWidth();

		ImGui::Text("Friction");
		ImGui::SameLine(Helper::g_Column);  ImGui::PushItemWidth(item_width); ImGui::InputFloat("##RigidBodyFriction", &friction, step, step_fast, precision, input_text_flags); ImGui::PopItemWidth();

		ImGui::Text("Rolling Friction");
		ImGui::SameLine(Helper::g_Column); ImGui::PushItemWidth(item_width); ImGui::InputFloat("##RigidBodyRollingFriction", &friction_rolling, step, step_fast, precision, input_text_flags); ImGui::PopItemWidth();

		ImGui::Text("Restitution");
		ImGui::SameLine(Helper::g_Column); ImGui::PushItemWidth(item_width); ImGui::InputFloat("##RigidBodyRestitution", &restitution, step, step_fast, precision, input_text_flags); ImGui::PopItemWidth();

		ImGui::Text("Use Gravity");
		ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##RigidBodyUseGravity", &use_gravity);

		ImGui::Text("Is Kinematic");
		ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##RigidBodyKinematic", &is_kinematic);

		ImGui::Text("Freeze Position");
		ImGui::SameLine(Helper::g_Column); ImGui::Text("X");
		ImGui::SameLine(); ImGui::Checkbox("##RigidFreezePosX", &freeze_pos_x);
		ImGui::SameLine(); ImGui::Text("Y");
		ImGui::SameLine(); ImGui::Checkbox("##RigidFreezePosY", &freeze_pos_y);
		ImGui::SameLine(); ImGui::Text("Z");
		ImGui::SameLine(); ImGui::Checkbox("##RigidFreezePosZ", &freeze_pos_z);

		ImGui::Text("Freeze Rotation");
		ImGui::SameLine(Helper::g_Column); ImGui::Text("X");
		ImGui::SameLine(); ImGui::Checkbox("##RigidFreezeRotX", &freeze_rot_x);
		ImGui::SameLine(); ImGui::Text("Y");
		ImGui::SameLine(); ImGui::Checkbox("##RigidFreezeRotY", &freeze_rot_y);
		ImGui::SameLine(); ImGui::Text("Z");
		ImGui::SameLine(); ImGui::Checkbox("##RigidFreezeRotZ", &freeze_rot_z);

		if (mass != rigid_body->GetMass())                                      rigid_body->SetMass(mass);
		if (friction != rigid_body->GetFriction())                              rigid_body->SetFriction(friction);
		if (friction_rolling != rigid_body->GetFrictionRolling())               rigid_body->SetFrictionRolling(friction_rolling);
		if (restitution != rigid_body->GetRestitution())                        rigid_body->SetRestitution(restitution);
		if (use_gravity != rigid_body->GetUseGravity())                         rigid_body->SetUseGravity(use_gravity);
		if (is_kinematic != rigid_body->GetIsKinematic())                       rigid_body->SetIsKinematic(is_kinematic);
		if (freeze_pos_x != static_cast<bool>(rigid_body->GetPositionLock().x)) rigid_body->SetPositionLock(Vector3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
		if (freeze_pos_y != static_cast<bool>(rigid_body->GetPositionLock().y)) rigid_body->SetPositionLock(Vector3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
		if (freeze_pos_z != static_cast<bool>(rigid_body->GetPositionLock().z)) rigid_body->SetPositionLock(Vector3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
		if (freeze_rot_x != static_cast<bool>(rigid_body->GetRotationLock().x)) rigid_body->SetRotationLock(Vector3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
		if (freeze_rot_y != static_cast<bool>(rigid_body->GetRotationLock().y)) rigid_body->SetRotationLock(Vector3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
		if (freeze_rot_z != static_cast<bool>(rigid_body->GetRotationLock().z)) rigid_body->SetRotationLock(Vector3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
	}

	Helper::ComponentEnd();
}


void Properties::ShowCollider(PlayGround::Collider* collider) const
{
	if (!collider)
		return;

	// 충돌체 시작
	if (Helper::ComponentBegin("Collider", "Collider", collider))
	{
		// 충돌체 타입
		static vector<string> shape_types = {
			"Box",
			"Sphere",
			"Static Plane",
			"Cylinder",
			"Capsule",
			"Cone",
			"Mesh"
		};

		// 최적화 여부
		bool optimize = collider->GetOptimize();
		// 충돌체 중심점
		Vector3 collider_center = collider->GetCenter();
		// 충돌체 바운딩 박스
		Vector3 collider_bounding_box = collider->GetBoundingBox();

		const auto input_text_flags = ImGuiInputTextFlags_CharsDecimal;
		const float step = 0.1f;
		const float step_fast = 0.1f;
		const char* precision = "%.3f";

		ImGui::Text("Type");
		ImGui::PushItemWidth(110.0f);
		ImGui::SameLine(Helper::g_Column);
		uint32_t selection_index = static_cast<uint32_t>(collider->GetShapeType());

		// 충돌체 타입 설정
		if (ImGuiEX::ComboBox("##ColliderType", shape_types, &selection_index))
		{
			collider->SetShapeType(static_cast<ColliderShape>(selection_index));
		}

		ImGui::PopItemWidth();

		ImGui::Text("Center");
		ImGui::PushItemWidth(110);
		ImGui::SameLine(Helper::g_Column); ImGui::PushID("colCenterX"); ImGui::InputFloat("X", &collider_center.x, step, step_fast, precision, input_text_flags); ImGui::PopID();
		ImGui::SameLine();                 ImGui::PushID("colCenterY"); ImGui::InputFloat("Y", &collider_center.y, step, step_fast, precision, input_text_flags); ImGui::PopID();
		ImGui::SameLine();                 ImGui::PushID("colCenterZ"); ImGui::InputFloat("Z", &collider_center.z, step, step_fast, precision, input_text_flags); ImGui::PopID();
		ImGui::PopItemWidth();

		ImGui::Text("Size");
		ImGui::PushItemWidth(110);
		ImGui::SameLine(Helper::g_Column); ImGui::PushID("colSizeX"); ImGui::InputFloat("X", &collider_bounding_box.x, step, step_fast, precision, input_text_flags); ImGui::PopID();
		ImGui::SameLine();                 ImGui::PushID("colSizeY"); ImGui::InputFloat("Y", &collider_bounding_box.y, step, step_fast, precision, input_text_flags); ImGui::PopID();
		ImGui::SameLine();                 ImGui::PushID("colSizeZ"); ImGui::InputFloat("Z", &collider_bounding_box.z, step, step_fast, precision, input_text_flags); ImGui::PopID();
		ImGui::PopItemWidth();

		if (collider->GetShapeType() == ColliderShape_Mesh)
		{
			ImGui::Text("Optimize");
			ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##colliderOptimize", &optimize);
		}

		if (collider_center != collider->GetCenter())            collider->SetCenter(collider_center);
		if (collider_bounding_box != collider->GetBoundingBox()) collider->SetBoundingBox(collider_bounding_box);
		if (optimize != collider->GetOptimize())                 collider->SetOptimize(optimize);
	}

	Helper::ComponentEnd();
}

void Properties::ShowMaterial(PlayGround::Material* material) const
{
	if (!material)
		return;

	// 마테리얼 컴포넌트 시작
	if (Helper::ComponentBegin("Material", "Material", nullptr, false))
	{
		const float offset_from_pos_x = 160;

		// UV 타일링, 오프셋
		Math::Vector2 tiling = material->GetTiling();
		Math::Vector2 offset = material->GetOffset();
		m_MaterialColorPicker->SetColor(material->GetColorAlbedo());

		ImGui::Text("Name");
		ImGui::SameLine(offset_from_pos_x); ImGui::Text(material->GetResourceName().c_str());

		if (material->IsEditable())
		{
			// 텍스쳐 슬롯
			{
				const auto show_property = [this, &offset_from_pos_x, &material](const char* name, const char* tooltip, const Material_Property type, bool show_texture, bool show_modifier)
				{
					// 이름
					if (name)
					{
						ImGui::Text(name);

						// 툴팁
						if (tooltip)
						{
							ImGuiEX::ToolTip(tooltip);
						}

						if (show_texture || show_modifier)
						{
							ImGui::SameLine(offset_from_pos_x);
						}
					}

					// 텍스쳐
					if (show_texture)
					{
						// setter 람다 함수
						auto setter = [&material, &type](const shared_ptr<RHI_Texture>& texture) { material->SetTextureSlot(type, texture); };
						// 이미지 슬롯과 콜백
						ImGuiEX::ImageSlot(material->GetTextureSharedPtr(type), setter);

						if (show_modifier)
						{
							ImGui::SameLine();
						}
					}

					if (show_modifier)
					{
						if (type == Material_Color)
						{
							m_MaterialColorPicker->Update();
						}
						else
						{
							ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));
							ImGuiEX::DragFloatWrap("", &material->GetProperty(type), 0.004f, 0.0f, 1.0f);
							ImGui::PopID();
						}
					}
				};

				show_property("Clearcoat", "Extra white specular layer on top of others", Material_Clearcoat, false, true);
				show_property("Clearcoat roughness", "Roughness of clearcoat specular", Material_Clearcoat_Roughness, false, true);
				show_property("Anisotropic", "Amount of anisotropy for specular reflection", Material_Anisotropic, false, true);
				show_property("Anisotropic rotation", "Rotates the direction of anisotropy, with 1.0 going full circle", Material_Anisotropic_Rotation, false, true);
				show_property("Sheen", "Amount of soft velvet like reflection near edges", Material_Sheen, false, true);
				show_property("Sheen tint", "Mix between white and using base color for sheen reflection", Material_Sheen_Tint, false, true);
				show_property("Color", "Diffuse or metal surface color", Material_Color, true, true);
				show_property("Roughness", "Specifies microfacet roughness of the surface for diffuse and specular reflection", Material_Roughness, true, true);
				show_property("Metallic", "Blends between a non-metallic and metallic material model", Material_Metallic, true, true);
				show_property("Normal", "Controls the normals of the base layers", Material_Normal, true, true);
				show_property("Height", "Perceived depth for parallax mapping", Material_Height, true, true);
				show_property("Occlusion", "Amount of light loss, can be complementary to SSAO", Material_Occlusion, true, false);
				show_property("Emission", "Light emission from the surface, works nice with bloom", Material_Emission, true, false);
				show_property("Alpha mask", "Discards pixels", Material_AlphaMask, true, false);
			}

			// UV
			{
				const float input_width = 128.0f;

				// 타일링
				ImGui::Text("Tiling");
				ImGui::SameLine(offset_from_pos_x); ImGui::Text("X");
				ImGui::PushItemWidth(input_width);
				ImGui::SameLine(); ImGui::InputFloat("##matTilingX", &tiling.x, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
				ImGui::SameLine(); ImGui::Text("Y");
				ImGui::SameLine(); ImGui::InputFloat("##matTilingY", &tiling.y, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
				ImGui::PopItemWidth();

				// 오프셋
				ImGui::Text("Offset");
				ImGui::SameLine(offset_from_pos_x); ImGui::Text("X");
				ImGui::PushItemWidth(input_width);
				ImGui::SameLine(); ImGui::InputFloat("##matOffsetX", &offset.x, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
				ImGui::SameLine(); ImGui::Text("Y");
				ImGui::SameLine(); ImGui::InputFloat("##matOffsetY", &offset.y, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
				ImGui::PopItemWidth();
			}
		}

		if (tiling != material->GetTiling())                                  material->SetTiling(tiling);
		if (offset != material->GetOffset())                                  material->SetOffset(offset);
		if (m_MaterialColorPicker->GetColor() != material->GetColorAlbedo()) material->SetColorAlbedo(m_MaterialColorPicker->GetColor());
	}

	Helper::ComponentEnd();
}

void Properties::ShowCamera(PlayGround::Camera* camera) const
{
	if (!camera)
		return;

	// 카메라 컴포넌트 시작
	if (Helper::ComponentBegin("Camera", "Camera", camera))
	{
		// 카메라 타입
		vector<string> projection_types = { "Perspective", "Orthographic" };
		float aperture = camera->GetAperture();
		float shutter_speed = camera->GetShutterSpeed();
		float iso = camera->GetIso();
		float fov = camera->GetFovHorizontalDeg();
		float near_plane = camera->GetNearPlane();
		float far_plane = camera->GetFarPlane();
		bool fps_control_enabled = camera->GetFpsControlEnabled();
		m_CameraColorPicker->SetColor(camera->GetClearColor());

		const auto input_text_flags = ImGuiInputTextFlags_CharsDecimal;

		ImGui::Text("Background");
		ImGui::SameLine(Helper::g_Column); m_CameraColorPicker->Update();

		ImGui::Text("Projection");
		ImGui::SameLine(Helper::g_Column);
		ImGui::PushItemWidth(115.0f);
		uint32_t selection_index = static_cast<uint32_t>(camera->GetProjectionType());
		if (ImGuiEX::ComboBox("##cameraProjection", projection_types, &selection_index))
		{
			camera->SetProjection(static_cast<ProjectionType>(selection_index));
		}
		ImGui::PopItemWidth();

		ImGui::SetCursorPosX(Helper::g_Column);
		ImGuiEX::DragFloatWrap("Aperture (mm)", &aperture, 0.01f, 0.01f, 150.0f);
		ImGuiEX::ToolTip("Size of the lens diaphragm. Controls depth of field and chromatic aberration.");

		ImGui::SetCursorPosX(Helper::g_Column);
		ImGuiEX::DragFloatWrap("Shutter Speed (sec)", &shutter_speed, 0.0001f, 0.0f, 1.0f, "%.4f");
		ImGuiEX::ToolTip("Length of time for which the camera shutter is open. Controls the amount of motion blur.");

		ImGui::SetCursorPosX(Helper::g_Column);
		ImGuiEX::DragFloatWrap("ISO", &iso, 0.1f, 0.0f, 2000.0f);
		ImGuiEX::ToolTip("Sensitivity to light. Controls camera noise.");

		ImGui::SetCursorPosX(Helper::g_Column);
		ImGuiEX::DragFloatWrap("Field of View", &fov, 0.1f, 1.0f, 179.0f);

		ImGui::Text("Clipping Planes");
		ImGui::SameLine(Helper::g_Column);      ImGui::PushItemWidth(130); ImGui::InputFloat("Near", &near_plane, 0.01f, 0.01f, "%.2f", input_text_flags); ImGui::PopItemWidth();
		ImGui::SetCursorPosX(Helper::g_Column); ImGui::PushItemWidth(130); ImGui::InputFloat("Far", &far_plane, 0.01f, 0.01f, "%.2f", input_text_flags); ImGui::PopItemWidth();

		ImGui::Text("FPS Control");
		ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##camera_fps_control", &fps_control_enabled);
		ImGuiEX::ToolTip("Enables FPS control while holding down the right mouse button");

		if (aperture != camera->GetAperture())                           camera->SetAperture(aperture);
		if (shutter_speed != camera->GetShutterSpeed())                  camera->SetShutterSpeed(shutter_speed);
		if (iso != camera->GetIso())                                     camera->SetIso(iso);
		if (fov != camera->GetFovHorizontalDeg())                        camera->SetFovHorizontalDeg(fov);
		if (near_plane != camera->GetNearPlane())                        camera->SetNearPlane(near_plane);
		if (far_plane != camera->GetFarPlane())                          camera->SetFarPlane(far_plane);
		if (fps_control_enabled != camera->GetFpsControlEnabled())       camera->SetFpsControlEnabled(fps_control_enabled);
		if (m_CameraColorPicker->GetColor() != camera->GetClearColor()) camera->SetClearColor(m_CameraColorPicker->GetColor());
	}

	Helper::ComponentEnd();
}

void Properties::ShowEnvironment(PlayGround::Environment* environment) const
{
	if (!environment)
		return;

	// 스카이박스 컴포넌트 시작
	if (Helper::ComponentBegin("Environment", "Environment", environment))
	{
		ImGui::Text("Sphere Map");

		ImGuiEX::ImageSlot(environment->GetTexture(), [&environment](const shared_ptr<RHI_Texture>& texture) { environment->SetTexture(texture); });
	}
	Helper::ComponentEnd();
}

void Properties::ShowAddComponentButton() const
{
	// 컴포넌트 추가 버튼

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 50);

	// 클릭시 팝업 오픈
	if (ImGuiEX::Button("Add Component"))
	{
		ImGui::OpenPopup("##ComponentContextMenu_Add");
	}
	ComponentContextMenu_Add();
}

void Properties::ComponentContextMenu_Add() const
{
	// 팝업 시작
	if (ImGui::BeginPopup("##ComponentContextMenu_Add"))
	{
		// 현재 선택된 엔티티 여부 확인
		if (auto entity = m_InspectedEntity.lock())
		{
			// 카메라
			if (ImGui::MenuItem("Camera"))
			{
				entity->AddComponent<Camera>();
			}

			// 라이트
			if (ImGui::BeginMenu("Light"))
			{
				if (ImGui::MenuItem("Directional"))
				{
					entity->AddComponent<Light>()->SetLightType(LightType::Directional);
				}
				else if (ImGui::MenuItem("Point"))
				{
					entity->AddComponent<Light>()->SetLightType(LightType::Point);
				}
				else if (ImGui::MenuItem("Spot"))
				{
					entity->AddComponent<Light>()->SetLightType(LightType::Spot);
				}

				ImGui::EndMenu();
			}

			// 물리
			if (ImGui::BeginMenu("Physics"))
			{
				if (ImGui::MenuItem("Rigid Body"))
				{
					entity->AddComponent<RigidBody>();
				}
				else if (ImGui::MenuItem("Collider"))
				{
					entity->AddComponent<Collider>();
				}

				ImGui::EndMenu();
			}

			// 스카이박스
			if (ImGui::BeginMenu("Environment"))
			{
				if (ImGui::MenuItem("Environment"))
				{
					entity->AddComponent<Environment>();
				}

				ImGui::EndMenu();
			}
		}

		ImGui::EndPopup();
	}
}
