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

// ���� ���õ� ��ƼƼ, ���׸���
weak_ptr<Entity> Properties::m_InspectedEntity;
weak_ptr<Material> Properties::m_InspectedMaterial;

// ������Ƽ ���� ���� �����̽�
namespace Helper
{
	static ResourceCache* resource_cache;
	static World* world;
	static Vector3 rotation_hint;

	static string g_ContextMenuID;
	static float g_Column = 180.0f;
	static const float g_MaxWidth = 100.0f;
	// ����� ������Ʈ
	static IComponent* g_Copied;

	// ������Ʈ �ɼ�
	inline void ComponentContextMenu_Options(const string& id, IComponent* component, const bool removable)
	{
		// �˾� ����
		if (ImGui::BeginPopup(id.c_str()))
		{
			// ������ �����ϴٸ�
			if (removable)
			{
				// ���� �޴�
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

			// ����
			if (ImGui::MenuItem("Copy Attributes"))
			{
				g_Copied = component;
			}

			if (ImGui::MenuItem("Paste Attributes"))
			{
				// ���� ����� ������Ʈ�� �����ϰ� ���� ����� ���� ������Ʈ Ÿ���̶��
				if (g_Copied && g_Copied->GetComponentType() == component->GetComponentType())
				{
					// ���� �����ذ���.
					component->SetAttributes(g_Copied->GetAttributes());
				}
			}

			ImGui::EndPopup();
		}
	}

	// ������Ʈ ����
	inline bool ComponentBegin(const string& name, const EIconType icon_enum, IComponent* component_instance, bool options = true, const bool removable = true)
	{
		const bool collapsed = ImGuiEX::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_DefaultOpen);

		ImGui::SameLine();
		ImGui::Spacing();
		ImGui::SameLine();

		// ������Ʈ �ɼ�
		if (options)
		{
			// ������ �ʺ�
			const float icon_width = 16.0f;
			// y ��ġ
			const float original_pen_y = ImGui::GetCursorPosY();

			// y ��ġ ����
			ImGui::SetCursorPosY(original_pen_y + 5.0f);
			// �̹��� ���
			ImGuiEX::Image(icon_enum, 15);
			ImGui::SameLine(ImGuiEX::GetWindowContentRegionWidth() - icon_width + 1.0f); ImGui::SetCursorPosY(original_pen_y);
			// ���� ���ο� �̹��� ��ư �߰�
			if (ImGuiEX::ImageButton(name.c_str(), EIconType::Component_Options, icon_width))
			{
				g_ContextMenuID = name;
				// �˾� ����
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
	// ������Ƽ ���� �ʱ�ȭ
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
	// ���� ���� �ε��� ������ �ʾҴٸ� �׳� ��ȯ�Ѵ�.
	if (m_Context->GetSubModule<World>()->IsLoading())
		return;

	// ���� �ʺ�
	ImGui::PushItemWidth(Helper::g_MaxWidth);

	// ���� ���� ���õ� ��ƼƼ�� �����Ѵٸ�
	if (!m_InspectedEntity.expired())
	{
		// ��ƼƼ ������Ʈ�� �����´�.
		shared_ptr<Entity> entity_ptr = m_InspectedEntity.lock();
		Renderable* renderable = entity_ptr->GetComponent<Renderable>();
		Material* material = renderable ? renderable->GetMaterial() : nullptr;

		// ��ƼƼ�� �������ִ� ������Ʈ ��� ���
		ShowTransform(entity_ptr->GetComponent<Transform>());
		ShowLight(entity_ptr->GetComponent<Light>());
		ShowCamera(entity_ptr->GetComponent<Camera>());
		ShowEnvironment(entity_ptr->GetComponent<Environment>());
		ShowRenderable(renderable);
		ShowMaterial(material);
		ShowRigidBody(entity_ptr->GetComponent<RigidBody>());
		ShowCollider(entity_ptr->GetComponent<Collider>());

		// ������Ʈ �߰���ư
		ShowAddComponentButton();
	}
	// ���� ���׸����� ��� ���׸��� ���� ���
	else if (!m_InspectedMaterial.expired())
	{
		ShowMaterial(m_InspectedMaterial.lock().get());
	}

	ImGui::PopItemWidth();
}

void Properties::Inspect(const weak_ptr<Entity>& entity)
{
	m_InspectedEntity = entity;

	// ���� ���õ� ��ƼƼ�� �ִٸ� ȸ�� ���� ���
	if (const auto shared_ptr = entity.lock())
	{
		Helper::rotation_hint = shared_ptr->GetTransform()->GetLocalRotation().ToEulerAngles();
	}
	else
	{
		Helper::rotation_hint = Vector3::Zero;
	}

	// ���� ����Ƽ���� ���õǾ� �ִٸ� ���׸��� ����
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
	// Ʈ������

	enum class Axis
	{
		x,
		y,
		z
	};

	// Ʈ������ ������Ʈ ����
	if (Helper::ComponentBegin("Transform", "Transform", transform, true, false))
	{
		// �÷��� ���� Ȯ��
		const bool is_playing = m_Context->m_Engine->IsEngineModeSet(GameMode);

		// ��ġ, ȸ��, ������ ��
		Vector3 position = transform->GetLocalPosition();
		Vector3 rotation = transform->GetLocalRotation().ToEulerAngles();
		Vector3 scale = transform->GetLocalScale();

		// �ε��Ҽ��� �� ������ ���� �Լ�
		const auto show_float = [](Axis axis, float* value)
		{
			// �����̽�
			const float label_float_spacing = 15.0f;
			// ���� ��ġ�� 0.01
			const float step = 0.01f;
			// �Ҽ��� 4��° �ڸ����� ǥ��
			const string format = "%.4f";

			// �� �� ���
			ImGui::TextUnformatted(axis == Axis::x ? "x" : axis == Axis::y ? "y" : "z");
			// �����̽�
			ImGui::SameLine(label_float_spacing);
			// �� ��ġ
			Vector2 pos_post_label = Vector2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);

			// �ʺ�
			ImGui::PushItemWidth(128.0f);
			ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));
			// �Ҽ��� �Է� ��ġ
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

		// ��ġ, ȸ��, ������ ������ ���� �����ִ� ���� �Լ�
		const auto show_vector = [&show_float](const char* label, Vector3& vector)
		{
			const float label_indetation = 15.0f;

			// �׷� ����
			ImGui::BeginGroup();
			ImGui::Indent(label_indetation);
			ImGui::TextUnformatted(label);
			ImGui::Unindent(label_indetation);
			// ������ �� 
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

		// ���� �÷��� ��尡 �ƴ϶��
		if (!is_playing)
		{
			// ������ ��ġ, ȸ��, ������ ��������
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

	// ����Ʈ ������Ʈ ����
	if (Helper::ComponentBegin("Light", "Light", light))
	{
		// ������ Ÿ��
		static vector<string> types = { "Directional", "Point", "Spot" };
		// ������ ����
		float intensity = light->GetIntensity();
		// ����
		float angle = light->GetAngle() * Math::Util::RAD_TO_DEG * 2.0f;
		// �׸��� ĳ���� ����
		bool shadows = light->GetShadowsEnabled();
		// ��ũ�� �׸��� ����
		bool shadows_screen_space = light->GetShadowsScreenSpaceEnabled();
		// �׸��� ���� ����
		bool shadows_transparent = light->GetShadowsTransparentEnabled();
		// ������Ʈ��
		bool volumetric = light->GetVolumetricEnabled();
		// ���̾
		float bias = light->GetBias();
		float normal_bias = light->GetNormalBias();
		float range = light->GetRange();
		// ���� ���� ����
		m_LightColorPicker->SetColor(light->GetColor());

		// ���⼺ ��������
		bool is_directional = light->GetLightType() == LightType::Directional;

		ImGui::Text("Type");
		ImGui::PushItemWidth(110.0f);
		ImGui::SameLine(Helper::g_Column);
		uint32_t selection_index = static_cast<uint32_t>(light->GetLightType());
		
		// ���� Ÿ�� ����
		if (ImGuiEX::ComboBox("##LightType", types, &selection_index))
		{
			light->SetLightType(static_cast<LightType>(selection_index));
		}

		ImGui::PopItemWidth();

		// ���� ����
		ImGui::Text("Color");
		ImGui::SameLine(Helper::g_Column); m_LightColorPicker->Update();

		// ����
		ImGui::Text(is_directional ? "Intensity (Lux)" : "Intensity (Lumens)");
		ImGui::SameLine(Helper::g_Column);
		float v_speed = is_directional ? 20.0f : 5.0f;
		float v_max = is_directional ? 128000.0f : 100000.0f;
		ImGui::PushItemWidth(300); ImGuiEX::DragFloatWrap("##LightIntensity", &intensity, v_speed, 0.0f, v_max); ImGui::PopItemWidth();

		// �׸��� ����
		ImGui::Text("Shadows");
		ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##LightShadows", &shadows);

		// �׸��� ������ ����������� ��Ȱ��ȭ
		ImGui::BeginDisabled(!shadows);
		{
			// ���� �׸���
			ImGui::Text("Transparent Shadows");
			ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##Light_shadows_transparent", &shadows_transparent);
			ImGuiEX::ToolTip("Allows transparent objects to cast colored translucent shadows");

			// ������Ʈ�� ������ �ɼ�
			ImGui::Text("Volumetric");
			ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##Light_volumetric", &volumetric);
			ImGuiEX::ToolTip("The shadow map is used to determine which parts of the \"air\" should be lit");
		}

		ImGui::EndDisabled();

		// ��ũ�� �����̽� �׸���
		ImGui::Text("Screen Space Shadows");
		ImGui::SameLine(Helper::g_Column); ImGui::Checkbox("##Light_shadows_screen_space", &shadows_screen_space);
		ImGuiEX::ToolTip("Small scale shadows which add detail were surfaces meet, also known as contact shadows");

		// ���̾
		ImGui::Text("Bias");
		ImGui::SameLine(Helper::g_Column);
		ImGui::PushItemWidth(300); ImGui::InputFloat("##LightBias", &bias, 1.0f, 1.0f, "%.0f"); ImGui::PopItemWidth();

		ImGui::Text("Normal Bias");
		ImGui::SameLine(Helper::g_Column);
		ImGui::PushItemWidth(300); ImGui::InputFloat("##LightNormalBias", &normal_bias, 1.0f, 1.0f, "%.0f"); ImGui::PopItemWidth();

		// ���⼺ ������ ���
		if (light->GetLightType() != LightType::Directional)
		{
			ImGui::Text("Range");
			ImGui::SameLine(Helper::g_Column);
			ImGui::PushItemWidth(300); ImGuiEX::DragFloatWrap("##LightRange", &range, 0.01f, 0.0f, 1000.0f); ImGui::PopItemWidth();
		}

		// ���� ����Ʈ�� ���
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

	// �������� ������Ʈ ����
	if (Helper::ComponentBegin("Renderable", "Renderer", renderable))
	{
		// �޽� �̸�
		const string& mesh_name = renderable->GeometryName();
		// ���׸���
		Material* material = renderable->GetMaterial();
		// ���׸��� �̸�
		string material_name = material ? material->GetResourceName() : "N/A";
		// �׸��� ���� ����
		bool cast_shadows = renderable->GetCastShadows();

		// �޽�
		ImGui::Text("Mesh");
		ImGui::SameLine(Helper::g_Column); ImGui::Text(mesh_name.c_str());

		// ���׸���
		ImGui::Text("Material");
		ImGui::SameLine(Helper::g_Column);
		ImGui::PushID("##MaterialName");
		ImGui::PushItemWidth(200.0f);
		ImGui::InputText("", &material_name, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

		// ���׸��� �巡�� ��� ���̷ε�
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

	// ������ �ٵ� ������Ʈ ����
	if (Helper::ComponentBegin("RigidBody", "Rigid Body", rigid_body))
	{
		// ����
		float mass = rigid_body->GetMass();
		// ����
		float friction = rigid_body->GetFriction();
		// ���� ����
		float friction_rolling = rigid_body->GetFrictionRolling();
		// ������
		float restitution = rigid_body->GetRestitution();
		// �߷� ����
		bool use_gravity = rigid_body->GetUseGravity();
		// Ű�׸�ƽ ����
		bool is_kinematic = rigid_body->GetIsKinematic();
		// ��ġ ����
		bool freeze_pos_x = static_cast<bool>(rigid_body->GetPositionLock().x);
		bool freeze_pos_y = static_cast<bool>(rigid_body->GetPositionLock().y);
		bool freeze_pos_z = static_cast<bool>(rigid_body->GetPositionLock().z);
		
		// ȸ�� ����
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

	// �浹ü ����
	if (Helper::ComponentBegin("Collider", "Collider", collider))
	{
		// �浹ü Ÿ��
		static vector<string> shape_types = {
			"Box",
			"Sphere",
			"Static Plane",
			"Cylinder",
			"Capsule",
			"Cone",
			"Mesh"
		};

		// ����ȭ ����
		bool optimize = collider->GetOptimize();
		// �浹ü �߽���
		Vector3 collider_center = collider->GetCenter();
		// �浹ü �ٿ�� �ڽ�
		Vector3 collider_bounding_box = collider->GetBoundingBox();

		const auto input_text_flags = ImGuiInputTextFlags_CharsDecimal;
		const float step = 0.1f;
		const float step_fast = 0.1f;
		const char* precision = "%.3f";

		ImGui::Text("Type");
		ImGui::PushItemWidth(110.0f);
		ImGui::SameLine(Helper::g_Column);
		uint32_t selection_index = static_cast<uint32_t>(collider->GetShapeType());

		// �浹ü Ÿ�� ����
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

	// ���׸��� ������Ʈ ����
	if (Helper::ComponentBegin("Material", "Material", nullptr, false))
	{
		const float offset_from_pos_x = 160;

		// UV Ÿ�ϸ�, ������
		Math::Vector2 tiling = material->GetTiling();
		Math::Vector2 offset = material->GetOffset();
		m_MaterialColorPicker->SetColor(material->GetColorAlbedo());

		ImGui::Text("Name");
		ImGui::SameLine(offset_from_pos_x); ImGui::Text(material->GetResourceName().c_str());

		if (material->IsEditable())
		{
			// �ؽ��� ����
			{
				const auto show_property = [this, &offset_from_pos_x, &material](const char* name, const char* tooltip, const Material_Property type, bool show_texture, bool show_modifier)
				{
					// �̸�
					if (name)
					{
						ImGui::Text(name);

						// ����
						if (tooltip)
						{
							ImGuiEX::ToolTip(tooltip);
						}

						if (show_texture || show_modifier)
						{
							ImGui::SameLine(offset_from_pos_x);
						}
					}

					// �ؽ���
					if (show_texture)
					{
						// setter ���� �Լ�
						auto setter = [&material, &type](const shared_ptr<RHI_Texture>& texture) { material->SetTextureSlot(type, texture); };
						// �̹��� ���԰� �ݹ�
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

				// Ÿ�ϸ�
				ImGui::Text("Tiling");
				ImGui::SameLine(offset_from_pos_x); ImGui::Text("X");
				ImGui::PushItemWidth(input_width);
				ImGui::SameLine(); ImGui::InputFloat("##matTilingX", &tiling.x, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
				ImGui::SameLine(); ImGui::Text("Y");
				ImGui::SameLine(); ImGui::InputFloat("##matTilingY", &tiling.y, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
				ImGui::PopItemWidth();

				// ������
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

	// ī�޶� ������Ʈ ����
	if (Helper::ComponentBegin("Camera", "Camera", camera))
	{
		// ī�޶� Ÿ��
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

	// ��ī�̹ڽ� ������Ʈ ����
	if (Helper::ComponentBegin("Environment", "Environment", environment))
	{
		ImGui::Text("Sphere Map");

		ImGuiEX::ImageSlot(environment->GetTexture(), [&environment](const shared_ptr<RHI_Texture>& texture) { environment->SetTexture(texture); });
	}
	Helper::ComponentEnd();
}

void Properties::ShowAddComponentButton() const
{
	// ������Ʈ �߰� ��ư

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 50);

	// Ŭ���� �˾� ����
	if (ImGuiEX::Button("Add Component"))
	{
		ImGui::OpenPopup("##ComponentContextMenu_Add");
	}
	ComponentContextMenu_Add();
}

void Properties::ComponentContextMenu_Add() const
{
	// �˾� ����
	if (ImGui::BeginPopup("##ComponentContextMenu_Add"))
	{
		// ���� ���õ� ��ƼƼ ���� Ȯ��
		if (auto entity = m_InspectedEntity.lock())
		{
			// ī�޶�
			if (ImGui::MenuItem("Camera"))
			{
				entity->AddComponent<Camera>();
			}

			// ����Ʈ
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

			// ����
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

			// ��ī�̹ڽ�
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
