#include "Common.h"
#include "Sphere.h"

namespace PlayGround::Math
{
	Sphere::Sphere(const Vector3& _center, const float _radius)
	{
		center = _center;
		radius = _radius;
	}
}