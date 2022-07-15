#pragma once

#include "../../Math/Matrix.h"
#include "../../Math/BoundingBox.h"
#include "TransformEnums.h"

namespace PlayGround
{
	class Renderer;
	class Context;
	class Input;
	class Transfrom;

	struct TransformOperatorAxis
	{
		TransformOperatorAxis() = default;
		TransformOperatorAxis(TransformHandleType type, const Math::Vector3& axis, Context* context);

		void UpdateTransform();
		void DrawPrimitives(const Math::Vector3& transform_center) const;
		const Math::Vector3& GetColor() const;

		Math::Vector3 m_Axis = Math::Vector3::One;
		Math::Matrix m_Transform = Math::Matrix::Identity;
		Math::Vector3 m_Position = Math::Vector3::One;
		Math::Quaternion m_Rotation = Math::Quaternion::Identity;
		Math::Vector3 m_Scale = Math::Vector3::One;
		Math::BoundingBox m_Box = Math::BoundingBox::Zero;
		Math::BoundingBox m_BoxTransformed = Math::BoundingBox::Zero;
		bool m_Is_editing = false;
		bool m_Is_hovered = false;
		bool m_Is_disabled = false;
		bool m_Is_editing_prev = false;
		bool m_Is_first_editing_run = false;
		Math::Vector3 m_ColorActive = Math::Vector3(1.0f, 1.0f, 0.0f);
		Math::Vector3 m_ColorDisabled = Math::Vector3(0.5f, 0.5f, 0.5f);
		TransformHandleType m_Type = TransformHandleType::Unknown;
		
		Context* m_Context = nullptr;
		Renderer* m_Renderer = nullptr;
		Input* m_Input = nullptr;
	};
}