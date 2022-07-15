#include "Common.h"
#include "Ray.h"
#include "BoundingBox.h"
#include "Plane.h"
#include "Sphere.h"

using namespace std;

namespace PlayGround::Math
{
    Ray::Ray(const Vector3& start, const Vector3& direction)
    {
        m_origin = start;
        m_direction = direction.Normalized();
    }

    float Ray::HitDistance(const BoundingBox& box) const
    {
        if (!box.Defined())
            return Util::INFINITY_;
        if (box.IsInside(m_origin) == Intersection::Inside)
            return 0.0f;

        float distance = Util::INFINITY_;

        if (m_origin.x < box.GetMin().x && m_direction.x > 0.0f)
        {
            const float x = (box.GetMin().x - m_origin.x) / m_direction.x;
            if (x < distance)
            {
                const Vector3 point = m_origin + x * m_direction;
                if (point.y >= box.GetMin().y && point.y <= box.GetMax().y && point.z >= box.GetMin().z && point.z <= box.GetMax().z)
                {
                    distance = x;
                }
            }
        }
        if (m_origin.x > box.GetMax().x && m_direction.x < 0.0f)
        {
            const float x = (box.GetMax().x - m_origin.x) / m_direction.x;
            if (x < distance)
            {
                const Vector3 point = m_origin + x * m_direction;
                if (point.y >= box.GetMin().y && point.y <= box.GetMax().y && point.z >= box.GetMin().z && point.z <= box.GetMax().z)
                {
                    distance = x;
                }
            }
        }

        if (m_origin.y < box.GetMin().y && m_direction.y > 0.0f)
        {
            const float x = (box.GetMin().y - m_origin.y) / m_direction.y;
            if (x < distance)
            {
                const Vector3 point = m_origin + x * m_direction;
                if (point.x >= box.GetMin().x && point.x <= box.GetMax().x && point.z >= box.GetMin().z && point.z <= box.GetMax().z)
                {
                    distance = x;
                }
            }
        }
        if (m_origin.y > box.GetMax().y && m_direction.y < 0.0f)
        {
            const float x = (box.GetMax().y - m_origin.y) / m_direction.y;
            if (x < distance)
            {
                const Vector3 point = m_origin + x * m_direction;
                if (point.x >= box.GetMin().x && point.x <= box.GetMax().x && point.z >= box.GetMin().z && point.z <= box.GetMax().z)
                {
                    distance = x;
                }
            }
        }

        if (m_origin.z < box.GetMin().z && m_direction.z > 0.0f)
        {
            const float x = (box.GetMin().z - m_origin.z) / m_direction.z;
            if (x < distance)
            {
                const Vector3 point = m_origin + x * m_direction;
                if (point.x >= box.GetMin().x && point.x <= box.GetMax().x && point.y >= box.GetMin().y && point.y <= box.GetMax().y)
                {
                    distance = x;
                }
            }
        }
        if (m_origin.z > box.GetMax().z && m_direction.z < 0.0f)
        {
            const float x = (box.GetMax().z - m_origin.z) / m_direction.z;
            if (x < distance)
            {
                const Vector3 point = m_origin + x * m_direction;
                if (point.x >= box.GetMin().x && point.x <= box.GetMax().x && point.y >= box.GetMin().y && point.y <= box.GetMax().y)
                {
                    distance = x;
                }
            }
        }

        return distance;
    }

    float Ray::HitDistance(const Plane& plane, Vector3* intersection_point /*= nullptr*/) const
    {
        float d = plane.normal.Dot(m_direction);
        if (Util::Abs(d) >= Util::EPSILON)
        {
            float t = -(plane.normal.Dot(m_origin) + plane.d) / d;
            if (t >= 0.0f)
            {
                if (intersection_point)
                {
                    *intersection_point = m_origin + t * m_direction;
                }
                return t;
            }
            else
            {
                return Util::INFINITY_;
            }
        }
        else
        {
            return Util::INFINITY_;
        }
    }

    float Ray::HitDistance(const Vector3& v1, const Vector3& v2, const Vector3& v3, Vector3* out_normal /*= nullptr*/, Vector3* out_bary /*= nullptr*/) const
    {
        // http://www.graphics.cornell.edu/pubs/1997/MT97.pdf
        Vector3 edge1(v2 - v1);
        Vector3 edge2(v3 - v1);

        Vector3 p(m_direction.Cross(edge2));
        float det = edge1.Dot(p);

        if (det >= Util::EPSILON)
        {
            Vector3 t(m_origin - v1);
            float u = t.Dot(p);
            if (u >= 0.0f && u <= det)
            {
                Vector3 q(t.Cross(edge1));
                float v = m_direction.Dot(q);
                if (v >= 0.0f && u + v <= det)
                {
                    float distance = edge2.Dot(q) / det;

                    if (distance >= 0.0f)
                    {
                        if (out_normal)
                            *out_normal = edge1.Cross(edge2);
                        if (out_bary)
                            *out_bary = Vector3(1 - (u / det) - (v / det), u / det, v / det);

                        return distance;
                    }
                }
            }
        }

        return Util::INFINITY_;
    }

    float Ray::HitDistance(const Sphere& sphere) const
    {
        Vector3 centeredOrigin = m_origin - sphere.center;
        float squaredRadius = sphere.radius * sphere.radius;

        if (centeredOrigin.LengthSquared() <= squaredRadius)
            return 0.0f;

        float a = m_direction.Dot(m_direction);
        float b = 2.0f * centeredOrigin.Dot(m_direction);
        float c = centeredOrigin.Dot(centeredOrigin) - squaredRadius;
        float d = b * b - 4.0f * a * c;

        if (d < 0.0f)
            return Util::INFINITY_;

        float dSqrt = sqrtf(d);
        float dist = (-b - dSqrt) / (2.0f * a);
        if (dist >= 0.0f)
            return dist;
        else
            return (-b + dSqrt) / (2.0f * a);
    }

    float Ray::Distance(const Vector3& point) const
    {
        const Vector3 closest_point = m_origin + (m_direction * (point - m_origin).Dot(m_direction));
        return (closest_point - point).Length();
    }

    float Ray::Distance(const Vector3& point, Vector3& closest_point) const
    {
        closest_point = m_origin + (m_direction * (point - m_origin).Dot(m_direction));
        return (closest_point - point).Length();
    }

    Vector3 Ray::ClosestPoint(const Ray& ray) const
    {
        // http://paulbourke.net/geometry/lineline3d/
        Vector3 p13 = m_origin - ray.m_origin;
        Vector3 p43 = ray.m_direction;
        Vector3 p21 = m_direction;

        float d1343 = p13.Dot(p43);
        float d4321 = p43.Dot(p21);
        float d1321 = p13.Dot(p21);
        float d4343 = p43.Dot(p43);
        float d2121 = p21.Dot(p21);

        float d = d2121 * d4343 - d4321 * d4321;
        if (Util::Abs(d) < Util::EPSILON)
            return m_origin;

        float n = d1343 * d4321 - d1321 * d4343;
        float a = n / d;

        return m_origin + a * m_direction;
    }
}