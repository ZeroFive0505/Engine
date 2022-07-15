#pragma once

#include "MathUtil.h"
#include "Vector3.h"


namespace PlayGround
{
	struct RHI_Vertex_PosTexNorTan;

	namespace Math
	{
        class Matrix;

        class BoundingBox
        {
        public:
            BoundingBox();

            BoundingBox(const Vector3& min, const Vector3& max);

            BoundingBox(const Vector3* vertices, const uint32_t point_count);

            BoundingBox(const RHI_Vertex_PosTexNorTan* vertices, const uint32_t vertex_count);

            ~BoundingBox() = default;

            BoundingBox& operator =(const BoundingBox& rhs) = default;

            inline Vector3 GetCenter() const { return (m_max + m_min) * 0.5f; }

            inline Vector3 GetSize() const { return m_max - m_min; }

            inline Vector3 GetExtents() const { return (m_max - m_min) * 0.5f; }

            Intersection IsInside(const Vector3& point) const;

            Intersection IsInside(const BoundingBox& box) const;

            BoundingBox Transform(const Matrix& transform) const;

            void Merge(const BoundingBox& box);

            inline const Vector3& GetMin() const { return m_min; }
            inline const Vector3& GetMax() const { return m_max; }

            inline void Undefine() { m_min = Vector3::NegInf; m_max = Vector3::Inf; }
            inline bool Defined() const { return m_min.x != INFINITY; }

            static const BoundingBox Zero;

        private:
            Vector3 m_min;
            Vector3 m_max;
		};
	}
}
