#include "Common.h"
#include "Renderer.h"
#include "../World/Components/Camera.h"
#include "../World/Components/Transform.h"
#include "../World/Components/Light.h"
#include "../World/Entity.h"
#include "../World/World.h"
#include "../World/Components/Renderable.h"
#include "../World/TransformHandle/TransformHandle.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
    void Renderer::DrawLine(const Vector3& from, const Vector3& to, const Vector4& color_from, const Vector4& color_to, const float duration /*= 0.0f*/, const bool depth /*= true*/)
    {
        uint32_t& index = depth ? m_lines_index_depth_on : m_lines_index_depth_off;

        uint32_t vertex_count = static_cast<uint32_t>(m_line_vertices.size());
        if (index + 2 >= vertex_count)
        {
            uint32_t new_vertex_count = vertex_count == 0 ? 32768 : vertex_count * 2;

            if (vertex_count != 0)
            {
                LOG_INFO("Line buffer can hold %d vertices but %d are needed, resizing the buffer to fit %d vertices.", vertex_count, index + 2, new_vertex_count);
            }

            m_lines_index_depth_off = numeric_limits<uint32_t>::max();
            m_lines_index_depth_on = (new_vertex_count / 2) - 1;      

            m_line_vertices.reserve(new_vertex_count);
            m_line_vertices.resize(new_vertex_count);

            m_lines_duration.reserve(new_vertex_count);
            m_lines_duration.resize(new_vertex_count);
        }

        {
            index++;
            m_line_vertices[index] = RHI_Vertex_PosCol(from, color_from);
            m_lines_duration[index] = duration;

            index++;
            m_line_vertices[index] = RHI_Vertex_PosCol(to, color_to);
            m_lines_duration[index] = duration;
        }
    }

    void Renderer::DrawTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const Vector4& color /*= DEBUG_COLOR*/, const float duration /*= 0.0f*/, bool depth /*= true*/)
    {
        DrawLine(v0, v1, color, color, duration, depth);
        DrawLine(v1, v2, color, color, duration, depth);
        DrawLine(v2, v0, color, color, duration, depth);
    }

    void Renderer::DrawRectangle(const Rectangle& rectangle, const Vector4& color /*= DebugColor*/, const float duration /*= 0.0f*/, bool depth /*= true*/)
    {
        const float cam_z = m_camera->GetTransform()->GetPosition().z + m_camera->GetNearPlane() + 5.0f;

        DrawLine(Vector3(rectangle.left, rectangle.top, cam_z), Vector3(rectangle.right, rectangle.top, cam_z), color, color, duration, depth);
        DrawLine(Vector3(rectangle.right, rectangle.top, cam_z), Vector3(rectangle.right, rectangle.bottom, cam_z), color, color, duration, depth);
        DrawLine(Vector3(rectangle.right, rectangle.bottom, cam_z), Vector3(rectangle.left, rectangle.bottom, cam_z), color, color, duration, depth);
        DrawLine(Vector3(rectangle.left, rectangle.bottom, cam_z), Vector3(rectangle.left, rectangle.top, cam_z), color, color, duration, depth);
    }

    void Renderer::DrawBox(const BoundingBox& box, const Vector4& color, const float duration /*= 0.0f*/, const bool depth /*= true*/)
    {
        const Vector3& min = box.GetMin();
        const Vector3& max = box.GetMax();

        DrawLine(Vector3(min.x, min.y, min.z), Vector3(max.x, min.y, min.z), color, color, duration, depth);
        DrawLine(Vector3(max.x, min.y, min.z), Vector3(max.x, max.y, min.z), color, color, duration, depth);
        DrawLine(Vector3(max.x, max.y, min.z), Vector3(min.x, max.y, min.z), color, color, duration, depth);
        DrawLine(Vector3(min.x, max.y, min.z), Vector3(min.x, min.y, min.z), color, color, duration, depth);
        DrawLine(Vector3(min.x, min.y, min.z), Vector3(min.x, min.y, max.z), color, color, duration, depth);
        DrawLine(Vector3(max.x, min.y, min.z), Vector3(max.x, min.y, max.z), color, color, duration, depth);
        DrawLine(Vector3(max.x, max.y, min.z), Vector3(max.x, max.y, max.z), color, color, duration, depth);
        DrawLine(Vector3(min.x, max.y, min.z), Vector3(min.x, max.y, max.z), color, color, duration, depth);
        DrawLine(Vector3(min.x, min.y, max.z), Vector3(max.x, min.y, max.z), color, color, duration, depth);
        DrawLine(Vector3(max.x, min.y, max.z), Vector3(max.x, max.y, max.z), color, color, duration, depth);
        DrawLine(Vector3(max.x, max.y, max.z), Vector3(min.x, max.y, max.z), color, color, duration, depth);
        DrawLine(Vector3(min.x, max.y, max.z), Vector3(min.x, min.y, max.z), color, color, duration, depth);
    }

    void Renderer::DrawCircle(const Vector3& center, const Vector3& axis, const float radius, uint32_t segment_count, const Vector4& color /*= DEBUG_COLOR*/, const float duration /*= 0.0f*/, const bool depth /*= true*/)
    {
        if (radius <= 0.0f)
            return;

        segment_count = Util::Max<uint32_t>(segment_count, 4);

        vector<Vector3> points;
        points.reserve(segment_count + 1);
        points.resize(segment_count + 1);

        float angle_step = Math::Util::PI_2 / (float)segment_count;
        for (uint32_t i = 0; i <= segment_count; i++)
        {
            float angle = (float)i * angle_step;
            if (axis.x != 0.0f)
            {
                points[i] = Vector3(center.x, cos(angle) * radius + center.y, sin(angle) * radius + center.z);
            }
            else if (axis.y != 0.0f)
            {
                points[i] = Vector3(cos(angle) * radius + center.x, center.y, sin(angle) * radius + center.z);
            }
            else
            {
                points[i] = Vector3(cos(angle) * radius + center.x, sin(angle) * radius + center.y, center.z);
            }
        }

        for (uint32_t i = 0; i <= segment_count - 1; i++)
        {
            DrawLine(points[i], points[i + 1], color, color, duration, depth);
        }
    }

    void Renderer::DrawSphere(const Vector3& center, float radius, uint32_t segment_count, const Vector4& color /*= DEBUG_COLOR*/, const float duration /*= 0.0f*/, const bool depth /*= true*/)
    {
        segment_count = Util::Max<uint32_t>(segment_count, 4);

        Vector3 Vertex1, Vertex2, Vertex3, Vertex4;
        const float AngleInc = 2.f * Util::PI / float(segment_count);
        uint32_t NumSegmentsY = segment_count;
        float Latitude = AngleInc;
        uint32_t NumSegmentsX;
        float Longitude;
        float SinY1 = 0.0f, CosY1 = 1.0f, SinY2, CosY2;
        float SinX, CosX;

        while (NumSegmentsY--)
        {
            SinY2 = Util::Sin(Latitude);
            CosY2 = Util::Cos(Latitude);

            Vertex1 = Vector3(SinY1, 0.0f, CosY1) * radius + center;
            Vertex3 = Vector3(SinY2, 0.0f, CosY2) * radius + center;
            Longitude = AngleInc;

            NumSegmentsX = segment_count;
            while (NumSegmentsX--)
            {
                SinX = Util::Sin(Longitude);
                CosX = Util::Cos(Longitude);

                Vertex2 = Vector3((CosX * SinY1), (SinX * SinY1), CosY1) * radius + center;
                Vertex4 = Vector3((CosX * SinY2), (SinX * SinY2), CosY2) * radius + center;

                DrawLine(Vertex1, Vertex2, color, color, duration, depth);
                DrawLine(Vertex1, Vertex3, color, color, duration, depth);

                Vertex1 = Vertex2;
                Vertex3 = Vertex4;
                Longitude += AngleInc;
            }
            SinY1 = SinY2;
            CosY1 = CosY2;
            Latitude += AngleInc;
        }
    }

    void Renderer::DrawDirectionalArrow(const Vector3& start, const Vector3& end, float arrow_size, const Vector4& color /*= DEBUG_COLOR*/, const float duration /*= 0.0f*/, const bool depth /*= true*/)
    {
        arrow_size = Util::Max<float>(0.1f, arrow_size);

        DrawLine(start, end, color, color, duration, depth);

        Vector3 Dir = (end - start);
        Dir.Normalize();
        Vector3 Up(0, 0, 1);
        Vector3 Right = Dir.Cross(Up);
        if (!Right.IsNormalized())
        {
            Dir.FindBestAxisVectors(Up, Right);
        }

        Matrix TM;
        TM.m00 = Dir.x;   TM.m01 = Dir.y;   TM.m02 = Dir.z;
        TM.m10 = Right.x; TM.m11 = Right.y; TM.m12 = Right.z;
        TM.m20 = Up.x;    TM.m21 = Up.y;    TM.m22 = Up.z;

        float arrow_sqrt = Util::Sqrt(arrow_size);
        Vector3 arrow_pos;
        DrawLine(end, end + TM * Vector3(-arrow_sqrt, arrow_sqrt, 0), color, color, duration, depth);
        DrawLine(end, end + TM * Vector3(-arrow_sqrt, -arrow_sqrt, 0), color, color, duration, depth);
    }

    void Renderer::DrawPlane(const Math::Plane& plane, const Math::Vector4& color /*= DEBUG_COLOR*/, const float duration /*= 0.0f*/, const bool depth /*= true*/)
    {
        Vector3 plane_origin = plane.normal * plane.d;
        DrawDirectionalArrow(plane_origin, plane_origin + plane.normal * 2.0f, 0.2f, color, duration, depth);

        Vector3 U, V;
        plane.normal.FindBestAxisVectors(U, V);
        static const float scale = 10000.0f;
        DrawLine(plane_origin - U * scale, plane_origin + U * scale, color, color, duration, depth);
        DrawLine(plane_origin - V * scale, plane_origin + V * scale, color, color, duration, depth);
    }

    void Renderer::Lines_PostMain(const double delta_time)
    {
        m_lines_index_depth_off = numeric_limits<uint32_t>::max(); 
        m_lines_index_depth_on = (static_cast<uint32_t>(m_line_vertices.size()) / 2) - 1;
    }

    void Renderer::Lines_PreMain()
    {
        if (m_options & Renderer::Option::Debug_PickingRay)
        {
            const auto& ray = m_camera->GetPickingRay();
            DrawLine(ray.GetStart(), ray.GetStart() + ray.GetDirection() * m_camera->GetFarPlane(), Vector4(0, 1, 0, 1));
        }

        if (m_options & Renderer::Option::Debug_Lights)
        {
            auto& lights = m_entities[ObjectType::Light];
            for (const auto& entity : lights)
            {
                const Entity* entity_selected = m_Context->GetSubModule<World>()->GetTransformHandle()->GetSelectedEntity();
                if (entity_selected && entity_selected->GetObjectID() == entity->GetObjectID())
                {
                    Light* light = entity->GetComponent<Light>();

                    if (light->GetLightType() == LightType::Directional)
                    {
                        Vector3 pos_start = light->GetTransform()->GetPosition();
                        Vector3 pos_end = -pos_start;
                        DrawLine(pos_start, pos_end);

                    }
                    else if (light->GetLightType() == LightType::Point)
                    {
                        Vector3 center = light->GetTransform()->GetPosition();
                        float radius = light->GetRange();
                        uint32_t segment_count = 64;

                        DrawCircle(center, Vector3::Up, radius, segment_count);
                        DrawCircle(center, Vector3::Right, radius, segment_count);
                        DrawCircle(center, Vector3::Forward, radius, segment_count);
                    }
                    else if (light->GetLightType() == LightType::Spot)
                    {
                        float opposite = light->GetRange() * Math::Util::Tan(light->GetAngle());

                        Vector3 pos_end_center = light->GetTransform()->GetForward() * light->GetRange();
                        Vector3 pos_end_up = pos_end_center + light->GetTransform()->GetUp() * opposite;
                        Vector3 pos_end_right = pos_end_center + light->GetTransform()->GetRight() * opposite;
                        Vector3 pos_end_down = pos_end_center + light->GetTransform()->GetDown() * opposite;
                        Vector3 pos_end_left = pos_end_center + light->GetTransform()->GetLeft() * opposite;

                        Vector3 pos_start = light->GetTransform()->GetPosition();
                        DrawLine(pos_start, pos_start + pos_end_center);
                        DrawLine(pos_start, pos_start + pos_end_up);
                        DrawLine(pos_start, pos_start + pos_end_right);
                        DrawLine(pos_start, pos_start + pos_end_down);
                        DrawLine(pos_start, pos_start + pos_end_left);
                    }
                }
            }
        }

        if (m_options & Renderer::Option::Debug_Aabb)
        {
            for (const auto& entity : m_entities[ObjectType::GeometryOpaque])
            {
                if (auto renderable = entity->GetRenderable())
                {
                    DrawBox(renderable->GetAabb(), Vector4(0.41f, 0.86f, 1.0f, 1.0f));
                }
            }

            for (const auto& entity : m_entities[ObjectType::GeometryTransparent])
            {
                if (auto renderable = entity->GetRenderable())
                {
                    DrawBox(renderable->GetAabb(), Vector4(0.41f, 0.86f, 1.0f, 1.0f));
                }
            }
        }
    }
}