#pragma once

#include "TransformOperator.h"


namespace PlayGround
{
	class TransformRotation : public TransformOperator
	{
	public:
		TransformRotation(Context* context = nullptr);
		~TransformRotation() = default;
	protected:
		void InteresectionTest(const Math::Ray& mouse_ray) override;
		void ComputeDelta(const Math::Ray& mouse_ray, const Camera* camera) override;
		void MapToTransform(Transform* transform, const TransformHandleSpace space) override;
	private:
		Math::Vector3 m_Initial_direction;
		Math::Vector3 m_Intersection_axis;
		float m_PrevAngle = 0.0f;
		float m_AngleDelta = 0.0f;
	};
}