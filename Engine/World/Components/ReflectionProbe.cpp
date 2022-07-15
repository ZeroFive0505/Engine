#include "Common.h"
#include "ReflectionProbe.h"
#include "Transform.h"
#include "Renderable.h"
#include "../../RHI/RHI_TextureCube.h"
#include "../../RHI/RHI_Texture2D.h"
#include "../../Rendering/Renderer.h"
#include "../../IO/FileStream.h"
#include "../../RHI/RHI_Device.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	ReflectionProbe::ReflectionProbe(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
	{
        // 값 getter, setter를 설정
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Resolution, uint32_t);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Extents, Vector3);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Update_interval_frames, uint32_t);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Update_face_count, uint32_t);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_FarPlane, float);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_NearPlane, float);

        CreateTextures();
	}

    void ReflectionProbe::Update(double delta_time)
    {
        // 업데이트를 타이밍이라면
        if (m_Frames_since_last_update >= m_Update_interval_frames)
        {
            // 만약 첫 업데이트가 아니라면
            if (!m_First_update)
            {
                m_Update_face_start_index += m_Update_face_count;

                if (m_Update_face_start_index + m_Update_face_count > 6)
                {
                    m_Update_face_start_index = 0;
                }
            }
            else
                m_Update_face_start_index = 0;

            m_First_update = false;
            m_Frames_since_last_update = 0;
            m_Needs_to_update = true;
        }
        else
            m_Needs_to_update = false;

        m_Frames_since_last_update++;

        if (!m_Needs_to_update)
            return;

        // 리버스 z 확인
        bool reverse_z_changed = false;
        bool reverse_z = m_Context->GetSubModule<Renderer>()->GetOption(Renderer::Option::ReverseZ);
        if (m_Prev_reverse_z != reverse_z)
        {
            m_Prev_reverse_z = reverse_z;
            reverse_z_changed = true;
        }

        // 만약 설정이 바뀌었다면 새롭게 투영행렬을 계산한다.
        if (reverse_z_changed)
            ComputeProjectionMatrix();


        if (m_Transform->HasPositionChangedThisFrame())
        {
            // 6면의 행렬을 만들어낸다.
            const Vector3 position = m_Transform->GetPosition();
            m_MatrixViews[0] = Matrix::CreateLookAtLH(position, position + Vector3::Right, Vector3::Up);       // x+
            m_MatrixViews[1] = Matrix::CreateLookAtLH(position, position + Vector3::Left, Vector3::Up);       // x-
            m_MatrixViews[2] = Matrix::CreateLookAtLH(position, position + Vector3::Up, Vector3::Backward); // y+
            m_MatrixViews[3] = Matrix::CreateLookAtLH(position, position + Vector3::Down, Vector3::Forward);  // y-
            m_MatrixViews[4] = Matrix::CreateLookAtLH(position, position + Vector3::Forward, Vector3::Up);       // z+
            m_MatrixViews[5] = Matrix::CreateLookAtLH(position, position + Vector3::Backward, Vector3::Up);       // z-

            m_AABB = BoundingBox(position - m_Extents, position + m_Extents);
        }

        if (reverse_z_changed || m_Transform->HasPositionChangedThisFrame())
        {
            // 만약 설정이 변경되었거나 위치가 변경되었을 경우 새롭게 투영 행렬을 만들어낸다.
            for (uint32_t i = 0; i < m_TextureColor->GetArrayLength(); i++)
            {
                const float far_plane = reverse_z ? m_NearPlane : m_FarPlane;
                m_Frustums[i] = Frustum(m_MatrixViews[i], m_MatrixProjection, m_FarPlane);
            }
        }
    }

    void ReflectionProbe::Serialize(FileStream* stream)
    {
        // 저장

        stream->Write(m_Resolution);
        stream->Write(m_Extents);
        stream->Write(m_Update_interval_frames);
        stream->Write(m_Update_face_count);
        stream->Write(m_NearPlane);
        stream->Write(m_FarPlane);
    }

    void ReflectionProbe::Deserialize(FileStream* stream)
    {
        // 불러오기

        stream->Read(&m_Resolution);
        stream->Read(&m_Extents);
        stream->Read(&m_Update_interval_frames);
        stream->Read(&m_Update_face_count);
        stream->Read(&m_NearPlane);
        stream->Read(&m_FarPlane);
    }

    bool ReflectionProbe::IsInViewFrustum(Renderable* renderable, uint32_t index) const
    {
        // 컬링 여부 확인
        const auto box = renderable->GetAabb();
        const auto center = box.GetCenter();
        const auto extents = box.GetExtents();

        return m_Frustums[index].IsVisible(center, extents);
    }

    void ReflectionProbe::SetResolution(const uint32_t resolution)
    {
        uint32_t new_value = Util::Clamp<uint32_t>(16, m_Context->GetSubModule<Renderer>()->GetRhiDevice()->GetMaxTextureCubeDimension(), resolution);

        if (m_Resolution == new_value)
            return;

        m_Resolution = new_value;

        CreateTextures();
    }

    void ReflectionProbe::SetExtents(const Math::Vector3& extents)
    {
        m_Extents = extents;
    }

    void ReflectionProbe::SetUpdateIntervalFrames(const uint32_t update_interval_frame)
    {
        m_Update_interval_frames = Util::Clamp<uint32_t>(0, 128, update_interval_frame);
    }

    void ReflectionProbe::SetUpdateFaceCount(const uint32_t update_face_count)
    {
        m_Update_face_count = Util::Clamp<uint32_t>(1, 6, update_face_count);
    }

    void ReflectionProbe::SetNearPlane(const float near_plane)
    {
        float new_value = Util::Clamp<float>(0.1f, 1000.0f, near_plane);

        if (m_NearPlane == new_value)
            return;

        m_NearPlane = new_value;

        ComputeProjectionMatrix();
        ComputeFrustum();
    }

    void ReflectionProbe::SetFarPlane(const float far_plane)
    {
        float new_value = Util::Clamp<float>(0.1f, 1000.0f, far_plane);

        if (m_FarPlane == new_value)
            return;

        m_FarPlane = new_value;

        ComputeProjectionMatrix();
        ComputeFrustum();
    }

    void ReflectionProbe::CreateTextures()
    {
        m_TextureColor = make_unique<RHI_TextureCube>(m_Context, m_Resolution, m_Resolution, RHI_Format_R8G8B8A8_Unorm, RHI_Texture_Rt_Color | RHI_Texture_Srv, "reflection_probe_color");
        m_TextureDepth = make_unique<RHI_Texture2D>(m_Context, m_Resolution, m_Resolution, 1, RHI_Format_D32_Float, RHI_Texture_Rt_DepthStencil | RHI_Texture_Srv, "reflection_probe_depth");
    }

    void ReflectionProbe::ComputeProjectionMatrix()
    {
        bool reverse_z = m_Context->GetSubModule<Renderer>()->GetOption(Renderer::Option::ReverseZ);
        const float near_plane = reverse_z ? m_FarPlane : m_NearPlane;
        const float far_plane = reverse_z ? m_NearPlane : m_FarPlane;
        // 90도
        const float fov = Util::PI_DIV_2;
        const float aspect_ratio = 1.0f;
        m_MatrixProjection = Matrix::CreatePerspectiveFieldOfViewLH(fov, aspect_ratio, near_plane, far_plane);
    }


    void ReflectionProbe::ComputeFrustum()
    {
        bool reverse_z = m_Context->GetSubModule<Renderer>()->GetOption(Renderer::Option::ReverseZ);

        for (uint32_t i = 0; i < m_TextureColor->GetArrayLength(); i++)
        {
            const float far_plane = reverse_z ? m_NearPlane : m_FarPlane;
            m_Frustums[i] = Frustum(m_MatrixViews[i], m_MatrixProjection, far_plane);
        }
    }
}