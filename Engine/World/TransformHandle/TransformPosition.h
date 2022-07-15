#pragma once

#include "TransformOperator.h"

namespace PlayGround
{
	class TransformPosition : public TransformOperator
	{
	public:
		TransformPosition(Context* context = nullptr);
		~TransformPosition() = default;
	protected:
		void InteresectionTest(const Math::Ray& mouse_ray) override;
		void ComputeDelta(const Math::Ray& mouse_ray, const Camera* camera) override;
		void MapToTransform(Transform* transform, const TransformHandleSpace space) override;
	private:
		Math::Vector3 m_PrevMouse_point_on_axis = Math::Vector3::Zero;
		Math::Vector3 m_Delta = Math::Vector3::Zero;
	};
}