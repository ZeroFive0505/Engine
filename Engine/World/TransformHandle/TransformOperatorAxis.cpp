#include "Common.h"
#include "TransformOperatorAxis.h"
#include "../../Input/Input.h"
#include "../../World/Components/Transform.h"
#include "../../Rendering/Renderer.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	TransformOperatorAxis::TransformOperatorAxis(TransformHandleType type, const Math::Vector3& axis, Context* context)
	{
		m_Axis = axis;
		m_Type = type;
		m_Context = context;
		m_Renderer = context->GetSubModule<Renderer>();
		m_Input = context->GetSubModule<Input>();
	}

	void TransformOperatorAxis::UpdateTransform()
	{
		if (m_Type == TransformHandleType::Unknown)
			return;

		m_Transform = Math::Matrix(m_Position, m_Rotation, m_Scale);
		m_BoxTransformed = m_Box.Transform(m_Transform);
	}

	void TransformOperatorAxis::DrawPrimitives(const Vector3& transform_center) const
	{
		if (m_Type == TransformHandleType::Unknown)
			return;

		if (m_Type == TransformHandleType::Rotation)
		{
			const Vector3 center = m_BoxTransformed.GetCenter();
			const float radius = m_Scale.Length() * 5.0f;
			const uint32_t segment_count = 64;
			const Vector4 color = Vector4(GetColor(), 1.0f);
			m_Renderer->DrawCircle(center, m_Axis, radius, segment_count, color, 0.0f, false);
		}
		else
		{
			const Vector4 color = Vector4(GetColor(), 1.0f);
			const Vector3 from = m_BoxTransformed.GetCenter();
			const Vector3 to = transform_center;
			m_Renderer->DrawLine(from, to, color, color, 0.0f, false);
		}
	}

	const PlayGround::Math::Vector3& TransformOperatorAxis::GetColor() const
	{
		if (m_Is_disabled)
			return m_ColorDisabled;

		if (m_Is_hovered || m_Is_editing)
			return m_ColorActive;

		return m_Axis;
	}
}
