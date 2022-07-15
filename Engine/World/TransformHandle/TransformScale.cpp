#include "Common.h"
#include "TransformScale.h"
#include "../../Utils/Geometry.h"
#include "../../Rendering/Model.h"
#include "../Components/Camera.h"
#include "../Components/Transform.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	TransformScale::TransformScale(Context* context) : TransformOperator(context, TransformHandleType::Scale)
	{
		vector<RHI_Vertex_PosTexNorTan> vertices;
		vector<uint32_t> indices;

		Utility::Geometry::CreateCube(&vertices, &indices);

		m_AxisModel = make_unique<Model>(m_Context);
		m_AxisModel->AppendGeometry(indices, vertices);
		m_AxisModel->UpdateGeometry();

		m_Handle_x = TransformOperatorAxis(m_Type, Vector3::Right, m_Context);
		m_Handle_y = TransformOperatorAxis(m_Type, Vector3::Up, m_Context);
		m_Handle_z = TransformOperatorAxis(m_Type, Vector3::Forward, m_Context);
		m_Handle_xyz = TransformOperatorAxis(m_Type, Vector3::One, m_Context);

		m_Handle_x.m_Box = BoundingBox(vertices.data(), static_cast<uint32_t>(vertices.size()));
		m_Handle_y.m_Box = m_Handle_x.m_Box;
		m_Handle_z.m_Box = m_Handle_x.m_Box;
		m_Handle_xyz.m_Box = m_Handle_x.m_Box;

		m_Offset_handle_axes_from_center = true;
	}

	void TransformScale::InteresectionTest(const Math::Ray& mouse_ray)
	{
		m_Handle_x_intersected = mouse_ray.HitDistance(m_Handle_x.m_BoxTransformed) != Math::Util::INFINITY_;
		m_Handle_y_intersected = mouse_ray.HitDistance(m_Handle_y.m_BoxTransformed) != Math::Util::INFINITY_;
		m_Handle_z_intersected = mouse_ray.HitDistance(m_Handle_z.m_BoxTransformed) != Math::Util::INFINITY_;
		m_Handle_xyz_intersected = mouse_ray.HitDistance(m_Handle_xyz.m_BoxTransformed) != Math::Util::INFINITY_;
	}

	static Vector3 GetMousePointOnAxis(const Camera* camera, const Ray& mouse_ray, const TransformOperatorAxis& axis_handle)
	{
		Vector3 normal = camera->GetTransform()->GetForward();
		float distance_from_origin = 0.0f;
		Plane screen_plane = Plane(normal, distance_from_origin);
		Vector3 plane_intersection_point = Vector3::Zero;
		mouse_ray.HitDistance(screen_plane, &plane_intersection_point);

		Vector3 closest_point = Vector3::Zero;
		Ray(Vector3::Zero, axis_handle.m_Axis).Distance(plane_intersection_point, closest_point);

		return closest_point;
	}

	void TransformScale::ComputeDelta(const Math::Ray& mouse_ray, const Camera* camera)
	{
		Vector3 mouse_point_on_axis = Vector3::Zero;
		float sign = 1.0f;

		if (m_Handle_x.m_Is_editing)
			mouse_point_on_axis = GetMousePointOnAxis(camera, mouse_ray, m_Handle_x);
		if (m_Handle_y.m_Is_editing)
			mouse_point_on_axis = GetMousePointOnAxis(camera, mouse_ray, m_Handle_y);
		if (m_Handle_z.m_Is_editing)
			mouse_point_on_axis = GetMousePointOnAxis(camera, mouse_ray, m_Handle_z);
		else if (m_Handle_xyz.m_Is_editing)
		{
			float x = GetMousePointOnAxis(camera, mouse_ray, m_Handle_x).x;
			float y = GetMousePointOnAxis(camera, mouse_ray, m_Handle_y).y;
			float z = GetMousePointOnAxis(camera, mouse_ray, m_Handle_z).z;

			mouse_point_on_axis.x = Math::Util::Max3(x, y, z);
			mouse_point_on_axis.y = mouse_point_on_axis.x;
			mouse_point_on_axis.z = mouse_point_on_axis.x;

			sign = static_cast<float>(Math::Util::Sign(Vector3::Dot(mouse_ray.GetDirection(), Vector3::Right)));
			LOG_INFO("%f", sign);
		}

		bool is_first_editing_run = m_Handle_x.m_Is_first_editing_run || m_Handle_y.m_Is_first_editing_run || m_Handle_z.m_Is_first_editing_run || m_Handle_xyz.m_Is_first_editing_run;
		m_Delta = !is_first_editing_run ? (mouse_point_on_axis - m_PrevMouse_point_on_axis) : 0.0f;
		m_PrevMouse_point_on_axis = mouse_point_on_axis;
	}

	void TransformScale::MapToTransform(Transform* transform, const TransformHandleSpace space)
	{
		ASSERT(transform != nullptr);

		if (space == TransformHandleSpace::World)
			transform->SetScale(transform->GetScale() + m_Delta);
		else
			transform->SetLocalScale(transform->GetLocalScale() + m_Delta);
	}
}