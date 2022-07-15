#pragma once

#include "Vector3.h"
#include "../EngineDefinition.h"

namespace PlayGround
{
	namespace Math
	{
		class RayHit;
		class BoundingBox;
		class Plane;
		class Sphere;

		class Ray
		{
        public:
            Ray() = default;
            Ray(const Vector3& start, const Vector3& direction);
            ~Ray() = default;

            float HitDistance(const BoundingBox& box) const;
            float HitDistance(const Plane& plane, Vector3* intersection_point = nullptr) const;
            float HitDistance(const Vector3& v1, const Vector3& v2, const Vector3& v3, Vector3* out_normal = nullptr, Vector3* out_bary = nullptr) const;
            float HitDistance(const Sphere& sphere) const;

            float Distance(const Vector3& point) const;
            float Distance(const Vector3& point, Vector3& closest_point) const;

            Vector3 ClosestPoint(const Ray& ray) const;

            inline const Vector3& GetStart()     const { return m_origin; }
            inline const Vector3& GetDirection() const { return m_direction; }
        private:
            Vector3 m_origin;
            Vector3 m_direction;
		};
	}
}