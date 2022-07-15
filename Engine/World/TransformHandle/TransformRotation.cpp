#include "Common.h"
#include "TransformRotation.h"
#include "../../Utils/Geometry.h"
#include "../../Input/Input.h"
#include "../Components/Camera.h"
#include "../Components/Transform.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	TransformRotation::TransformRotation(Context* context) : TransformOperator(context, TransformHandleType::Rotation)
	{
		m_Handle_x = TransformOperatorAxis(m_Type, Vector3::Right, m_Context);
		m_Handle_y = TransformOperatorAxis(m_Type, Vector3::Up, m_Context);
		m_Handle_z = TransformOperatorAxis(m_Type, Vector3::Forward, m_Context);

		m_Offset_handle_axes_from_center = false;
	}

	void TransformRotation::InteresectionTest(const Math::Ray& mouse_ray)
	{
		const float circle_radius = m_Handle_x.m_Scale.Length() * 5.0f;
		const float circle_thickness = 0.05f;

		const Plane plane_x = Plane(m_Handle_x.m_Axis, 0.0f);
		const Plane plane_y = Plane(m_Handle_y.m_Axis, 0.0f);
		const Plane plane_z = Plane(m_Handle_z.m_Axis, 0.0f);

		Vector3 plane_x_intersection_point = Vector3::Inf;
		mouse_ray.HitDistance(plane_x, &plane_x_intersection_point);

		Vector3 plane_y_intersection_point = Vector3::Inf;
		mouse_ray.HitDistance(plane_y, &plane_y_intersection_point);

		Vector3 plane_z_intersection_point = Vector3::Inf;
		mouse_ray.HitDistance(plane_z, &plane_z_intersection_point);

		const float handle_x_distance = plane_x_intersection_point.Distance(m_Handle_x.m_Position);
		const float handle_y_distance = plane_y_intersection_point.Distance(m_Handle_y.m_Position);
		const float handle_z_distance = plane_z_intersection_point.Distance(m_Handle_z.m_Position);

		m_Handle_x_intersected = handle_x_distance >= (circle_radius - circle_thickness) &&
			handle_x_distance < (circle_radius + circle_thickness);
		m_Handle_y_intersected = handle_y_distance >= (circle_radius - circle_thickness) && 
			handle_y_distance < (circle_radius + circle_thickness);
		m_Handle_z_intersected = handle_z_distance >= (circle_radius - circle_thickness) && 
			handle_z_distance < (circle_radius + circle_thickness);

		if (m_Input->GetKeyDown(EKeyCode::Click_Left))
		{
			if (m_Handle_x_intersected)
			{
				m_Initial_direction = (plane_x_intersection_point - m_Position).Normalized();
				m_Intersection_axis = m_Handle_x.m_Axis;
			}
			else if (m_Handle_y_intersected)
			{
				m_Initial_direction = (plane_y_intersection_point - m_Position).Normalized();
				m_Intersection_axis = m_Handle_y.m_Axis;
			}
			else if (m_Handle_z_intersected)
			{
				m_Initial_direction = (plane_z_intersection_point - m_Position).Normalized();
				m_Intersection_axis = m_Handle_z.m_Axis;
			}
		}
	}

	void TransformRotation::ComputeDelta(const Math::Ray& mouse_ray, const Camera* camera)
	{
		const Plane plane = Plane(m_Intersection_axis, 0.0f);
		Vector3 plane_intersection_point = Vector3::Inf;
		mouse_ray.HitDistance(plane, &plane_intersection_point);

		Vector3 dir = (plane_intersection_point - m_Position).Normalized();
		float angle = Vector3::Dot(dir, m_Initial_direction);

		bool is_first_editing_run = m_Handle_x.m_Is_first_editing_run || m_Handle_y.m_Is_first_editing_run ||
			m_Handle_z.m_Is_first_editing_run || m_Handle_xyz.m_Is_first_editing_run;
		m_AngleDelta = !is_first_editing_run ? (angle - m_PrevAngle) : 0.0f;
		m_PrevAngle = angle;
	}

	void TransformRotation::MapToTransform(Transform* transform, const TransformHandleSpace space)
	{
		ASSERT(transform != nullptr);

		Quaternion rotation_delta = Quaternion::FromEulerAngles(Vector3(m_AngleDelta * Math::Util::RAD_TO_DEG) * m_Intersection_axis);

		if (space == TransformHandleSpace::World)
		{
			Quaternion rotation_new = transform->GetRotation() * rotation_delta;
			transform->SetRotation(rotation_new);
		}
		else
		{
			Quaternion rotation_new = transform->GetLocalRotation() * rotation_delta;
			transform->SetLocalRotaion(rotation_new);
		}
	}
}
