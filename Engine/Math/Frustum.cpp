#include "Common.h"
#include "Frustum.h"
#include "Matrix.h"
#include "Plane.h"

using namespace std;

namespace PlayGround::Math
{
    Frustum::Frustum(const Matrix& view, const Matrix& projection, float screen_depth)
    {
        // 프러스텀에서 최소 z 거리를 구한다.
        const float z_min = -projection.m32 / projection.m22;
        const float r = screen_depth / (screen_depth - z_min);
        Matrix projection_updated = projection;
        projection_updated.m22 = r;
        projection_updated.m32 = -r * z_min;

        const Matrix view_projection = view * projection_updated;

        // near plane 계산
        m_planes[0].normal.x = view_projection.m03 + view_projection.m02;
        m_planes[0].normal.y = view_projection.m13 + view_projection.m12;
        m_planes[0].normal.z = view_projection.m23 + view_projection.m22;
        m_planes[0].d = view_projection.m33 + view_projection.m32;
        m_planes[0].Normalize();

        // far plane 계산
        m_planes[1].normal.x = view_projection.m03 - view_projection.m02;
        m_planes[1].normal.y = view_projection.m13 - view_projection.m12;
        m_planes[1].normal.z = view_projection.m23 - view_projection.m22;
        m_planes[1].d = view_projection.m33 - view_projection.m32;
        m_planes[1].Normalize();

        // left plane 계산
        m_planes[2].normal.x = view_projection.m03 + view_projection.m00;
        m_planes[2].normal.y = view_projection.m13 + view_projection.m10;
        m_planes[2].normal.z = view_projection.m23 + view_projection.m20;
        m_planes[2].d = view_projection.m33 + view_projection.m30;
        m_planes[2].Normalize();

        // right plane 계산
        m_planes[3].normal.x = view_projection.m03 - view_projection.m00;
        m_planes[3].normal.y = view_projection.m13 - view_projection.m10;
        m_planes[3].normal.z = view_projection.m23 - view_projection.m20;
        m_planes[3].d = view_projection.m33 - view_projection.m30;
        m_planes[3].Normalize();

        // top plane 계산
        m_planes[4].normal.x = view_projection.m03 - view_projection.m01;
        m_planes[4].normal.y = view_projection.m13 - view_projection.m11;
        m_planes[4].normal.z = view_projection.m23 - view_projection.m21;
        m_planes[4].d = view_projection.m33 - view_projection.m31;
        m_planes[4].Normalize();

        // bottom plane 계산
        m_planes[5].normal.x = view_projection.m03 + view_projection.m01;
        m_planes[5].normal.y = view_projection.m13 + view_projection.m11;
        m_planes[5].normal.z = view_projection.m23 + view_projection.m21;
        m_planes[5].d = view_projection.m33 + view_projection.m31;
        m_planes[5].Normalize();
    }

    bool Frustum::IsVisible(const Vector3& center, const Vector3& extent, bool ignore_near_plane /*= false*/) const
    {
        float radius = 0.0f;

        if (!ignore_near_plane)
        {
            radius = Util::Max3(extent.x, extent.y, extent.z);
        }
        else
        {
            constexpr float z = numeric_limits<float>::infinity();
            radius = Util::Max3(extent.x, extent.y, z);
        }

        if (CheckSphere(center, radius) != Intersection::Outside)
            return true;

        if (CheckCube(center, radius) != Intersection::Outside)
            return true;

        return false;
    }

    Intersection Frustum::CheckCube(const Vector3& center, const Vector3& extent) const
    {
        Intersection result = Intersection::Inside;
        Plane plane_abs;

        // 주어진 육면체와 프러스텀이 교차하는지 확인한다.
        for (const Plane& plane : m_planes)
        {
            plane_abs.normal = plane.normal.Abs();
            plane_abs.d = plane.d;

            const float d = center.x * plane.normal.x + center.y * plane.normal.y + center.z * plane.normal.z;
            const float r = extent.x * plane_abs.normal.x + extent.y * plane_abs.normal.y + extent.z * plane_abs.normal.z;

            const float d_p_r = d + r;
            const float d_m_r = d - r;

            if (d_p_r < -plane.d)
            {
                result = Intersection::Outside;
                break;
            }

            if (d_m_r < -plane.d)
            {
                result = Intersection::Intersects;
            }
        }

        return result;
    }

    Intersection Frustum::CheckSphere(const Vector3& center, float radius) const
    {
        // 구와 교차하는지 확인한다.
        for (const auto& plane : m_planes)
        {
            const float distance = Vector3::Dot(plane.normal, center) + plane.d;

            if (distance < -radius)
                return Intersection::Outside;

            if (static_cast<float>(Util::Abs(distance)) < radius)
                return Intersection::Intersects;
        }

        return Intersection::Inside;
    }
}
