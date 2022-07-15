#pragma once

#include "Vector3.h"

namespace PlayGround::Math
{
	class Sphere
	{
	public:
		Sphere(const Vector3& center, const float radius);
		~Sphere() = default;

		Vector3 center;
		float radius;
	};
}