#include "Common.h"
#include "TransformOperator.h"
#include "../../Input/Input.h"
#include "../../World/Entity.h"
#include "../../World/Components/Camera.h"
#include "../../World/Components/Transform.h"
#include "../../World/Components/Renderable.h"
#include "../../Rendering/Model.h"
#include "../../Rendering/Renderer.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	TransformOperator::TransformOperator(Context* context, const TransformHandleType transform_handle_type)
	{
		m_Context = context;
		m_Type = transform_handle_type;
		m_Renderer = context->GetSubModule<Renderer>();
		m_Input = context->GetSubModule<Input>();
	}

	void TransformOperator::Update(const TransformHandleSpace space, Entity* entity, Camera* camera, const float handle_size)
	{
		ASSERT(entity != nullptr);
		ASSERT(camera != nullptr);

		SnapToTransform(space, entity, camera, handle_size);

		if (!camera->IsFpsControlled())
		{
			Vector3 ray_start = camera->GetTransform()->GetPosition();
			Vector3 ray_direction = camera->ScreenToWorldCoordinates(m_Input->GetMousePositionRelativeToEditorViewport(), 1.0f);
			Ray mouse_ray = Ray(ray_start, ray_direction);

			InteresectionTest(mouse_ray);

			{
                // Mark a handle as hovered, only if it's the only hovered handle (during the previous frame).
                m_Handle_x.m_Is_hovered = m_Handle_x_intersected && 
                    !(m_Handle_y.m_Is_hovered || m_Handle_z.m_Is_hovered);

                m_Handle_y.m_Is_hovered = m_Handle_y_intersected &&
                    !(m_Handle_x.m_Is_hovered || m_Handle_z.m_Is_hovered);

                m_Handle_z.m_Is_hovered = m_Handle_z_intersected &&
                    !(m_Handle_x.m_Is_hovered || m_Handle_y.m_Is_hovered);

                m_Handle_xyz.m_Is_hovered = m_Handle_xyz_intersected &&
                    !(m_Handle_x.m_Is_hovered || m_Handle_y.m_Is_hovered || m_Handle_z.m_Is_hovered);

                // Disable handle if one of the others is active (disabling turns the color of the handle to grey).
                m_Handle_x.m_Is_disabled = !m_Handle_x.m_Is_editing &&
                    (m_Handle_y.m_Is_editing || m_Handle_z.m_Is_editing || m_Handle_xyz.m_Is_editing);

                m_Handle_y.m_Is_disabled = !m_Handle_y.m_Is_editing &&
                    (m_Handle_x.m_Is_editing || m_Handle_z.m_Is_editing || m_Handle_xyz.m_Is_editing);

                m_Handle_z.m_Is_disabled = !m_Handle_z.m_Is_editing && 
                    (m_Handle_x.m_Is_editing || m_Handle_y.m_Is_editing || m_Handle_xyz.m_Is_editing);

                m_Handle_xyz.m_Is_disabled = !m_Handle_xyz.m_Is_editing &&
                    (m_Handle_x.m_Is_editing || m_Handle_y.m_Is_editing || m_Handle_z.m_Is_editing);

                // Keep old editing state
                m_Handle_x.m_Is_editing_prev = m_Handle_x.m_Is_editing;
                m_Handle_y.m_Is_editing_prev = m_Handle_y.m_Is_editing;
                m_Handle_z.m_Is_editing_prev = m_Handle_z.m_Is_editing;
                m_Handle_xyz.m_Is_editing_prev = m_Handle_xyz.m_Is_editing;

                // Detect if any of the handles should enter an editing state (on left click pressed)
                bool mouse_down = m_Input->GetKeyDown(EKeyCode::Click_Left);
                m_Handle_x.m_Is_editing = (m_Handle_x.m_Is_hovered && mouse_down) ? true : m_Handle_x.m_Is_editing;
                m_Handle_y.m_Is_editing = (m_Handle_y.m_Is_hovered && mouse_down) ? true : m_Handle_y.m_Is_editing;
                m_Handle_z.m_Is_editing = (m_Handle_z.m_Is_hovered && mouse_down) ? true : m_Handle_z.m_Is_editing;
                m_Handle_xyz.m_Is_editing = (m_Handle_xyz.m_Is_hovered && mouse_down) ? true : m_Handle_xyz.m_Is_editing;

                // Detect if any of the handles should exit the editing state (on left click released).
                bool mouse_up = m_Input->GetKeyUp(EKeyCode::Click_Left);
                m_Handle_x.m_Is_editing = (m_Handle_x.m_Is_editing && mouse_up) ? false : m_Handle_x.m_Is_editing;
                m_Handle_y.m_Is_editing = (m_Handle_y.m_Is_editing && mouse_up) ? false : m_Handle_y.m_Is_editing;
                m_Handle_z.m_Is_editing = (m_Handle_z.m_Is_editing && mouse_up) ? false : m_Handle_z.m_Is_editing;
                m_Handle_xyz.m_Is_editing = (m_Handle_xyz.m_Is_editing && mouse_up) ? false : m_Handle_xyz.m_Is_editing;

                // Determine if this is the first editing run
                m_Handle_x.m_Is_first_editing_run = !m_Handle_x.m_Is_editing_prev && m_Handle_x.m_Is_editing;
                m_Handle_y.m_Is_first_editing_run = !m_Handle_y.m_Is_editing_prev && m_Handle_y.m_Is_editing;
                m_Handle_z.m_Is_first_editing_run = !m_Handle_z.m_Is_editing_prev && m_Handle_z.m_Is_editing;
                m_Handle_xyz.m_Is_first_editing_run = !m_Handle_xyz.m_Is_editing_prev && m_Handle_xyz.m_Is_editing;
			}

            if (m_Handle_x.m_Is_editing || m_Handle_y.m_Is_editing || m_Handle_z.m_Is_editing || m_Handle_xyz.m_Is_editing)
            {
                ComputeDelta(mouse_ray, camera);

                MapToTransform(entity->GetTransform(), space);
            }
		}

        Vector3 center = m_Handle_xyz.m_Position;
        m_Handle_x.DrawPrimitives(center);
        m_Handle_y.DrawPrimitives(center);
        m_Handle_z.DrawPrimitives(center);
        m_Handle_xyz.DrawPrimitives(center);
	}

    void TransformOperator::SnapToTransform(const TransformHandleSpace space, Entity* entity, Camera* camera, const float handle_size)
    {
        Transform* entity_transform = entity->GetTransform();
        Renderable* entity_renderable = entity->GetComponent<Renderable>();

        const Vector3& center = entity_renderable ? entity_renderable->GetAabb().GetCenter() : entity_transform->GetLocalPosition();
        const Quaternion& entity_rotation = (space == TransformHandleSpace::World) ? entity_transform->GetRotation() : entity_transform->GetLocalRotation();
        const Vector3& right = (space == TransformHandleSpace::World) ? Vector3::Right : entity_rotation * Vector3::Right;
        const Vector3& up = (space == TransformHandleSpace::World) ? Vector3::Up : entity_rotation * Vector3::Up;
        const Vector3& forward = (space == TransformHandleSpace::World) ? Vector3::Forward : entity_rotation * Vector3::Forward;

        const float distance_to_camera = camera ? (camera->GetTransform()->GetPosition() - (center)).Length() : 0.0f;
        const float handle_scale = distance_to_camera / (1.0f / handle_size);
        m_Offset_handle_from_center = distance_to_camera / (1.0f / 0.1f);

        m_Handle_x.m_Position = center;
        m_Handle_y.m_Position = center;
        m_Handle_z.m_Position = center;
        m_Handle_xyz.m_Position = center;

        if (m_Offset_handle_axes_from_center)
        {
            m_Handle_x.m_Position += right * m_Offset_handle_from_center;
            m_Handle_y.m_Position += up * m_Offset_handle_from_center;
            m_Handle_z.m_Position += forward * m_Offset_handle_from_center;
        }

        m_Handle_x.m_Rotation = Quaternion::FromEulerAngles(0.0f, 0.0f, -90.0f);
        m_Handle_y.m_Rotation = Quaternion::FromEulerAngles(0.0f, 90.0f, 0.0f);
        m_Handle_z.m_Rotation = Quaternion::FromEulerAngles(90.0f, 0.0f, 0.0f);

        m_Handle_x.m_Scale = handle_scale;
        m_Handle_y.m_Scale = handle_scale;
        m_Handle_z.m_Scale = handle_scale;
        m_Handle_xyz.m_Scale = handle_scale;

        m_Handle_x.UpdateTransform();
        m_Handle_y.UpdateTransform();
        m_Handle_z.UpdateTransform();
        m_Handle_xyz.UpdateTransform();
    }

    const Matrix& TransformOperator::GetTransform(const Vector3& axis) const
    {
        if (axis == Vector3::Right)
            return m_Handle_x.m_Transform;
        else if (axis == Vector3::Up)
            return m_Handle_y.m_Transform;
        else if (axis == Vector3::Forward)
            return m_Handle_z.m_Transform;

        return m_Handle_xyz.m_Transform;
    }

    const Vector3& TransformOperator::GetColor(const Vector3& axis) const
    {
        if (axis == Vector3::Right)
            return m_Handle_x.GetColor();
        else if (axis == Vector3::Up)
            return m_Handle_y.GetColor();
        else if (axis == Vector3::Forward)
            return m_Handle_z.GetColor();

        return m_Handle_xyz.GetColor();
    }

    const RHI_VertexBuffer* TransformOperator::GetVertexBuffer()
    {
        return m_AxisModel ? m_AxisModel->GetVertexBuffer() : nullptr;
    }

    const RHI_IndexBuffer* TransformOperator::GetIndexBuffer()
    {
        return m_AxisModel ? m_AxisModel->GetIndexBuffer() : nullptr;
    }

    bool TransformOperator::IsEditing() const
    {
        ASSERT(m_Handle_x.m_Type != TransformHandleType::Unknown);

        return m_Handle_x.m_Is_editing || m_Handle_y.m_Is_editing || m_Handle_z.m_Is_editing || m_Handle_xyz.m_Is_editing;
    }

    bool TransformOperator::IsHovered() const
    {
        if (m_Handle_x.m_Type != TransformHandleType::Unknown && m_Handle_x.m_Is_hovered)
            return true;

        if (m_Handle_y.m_Type != TransformHandleType::Unknown && m_Handle_y.m_Is_hovered)
            return true;

        if (m_Handle_z.m_Type != TransformHandleType::Unknown && m_Handle_z.m_Is_hovered)
            return true;

        if (m_Handle_xyz.m_Type != TransformHandleType::Unknown && m_Handle_xyz.m_Is_hovered)
            return true;

        return false;
    }
}
