#include "Common.h"
#include "Light.h"
#include "Transform.h"
#include "Camera.h"
#include "Renderable.h"
#include "../World.h"
#include "../../IO/FileStream.h"
#include "../../Rendering/Renderer.h"
#include "../../RHI/RHI_Texture2D.h"
#include "../../RHI/RHI_TextureCube.h"
#include "../../RHI/RHI_Texture2DArray.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
    Light::Light(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
    {
        // ���� �� getter, setter ����
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_ShadowsEnabled, bool);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Shadows_screen_space_enabled, bool);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Shadows_transparent_enabled, bool);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Range, float);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Intensity, float);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_AngleRadian, float);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_ColorRGB, PlayGround::Math::Vector4);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Bias, float);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_NormalBias, float);
        REGISTER_ATTRIBUTE_GET_SET(GetLightType, SetLightType, LightType);

        // ������ ����
        m_Renderer = m_Context->GetSubModule<Renderer>();
    }

    void Light::OnInit()
    {

    }

    void Light::OnStart()
    {

    }

    void Light::Update(double delta_time)
    {
        ASSERT(m_Renderer != nullptr);

        // �ʱ�ȭ ���� �ʾҴٸ� �׸��� ���� ������.
        if (!m_Initialized)
        {
            CreateShadowMap();
            m_Initialized = true;
        }

        // ��Ƽ üũ
        {
            // z������ �ٲ���ų�, ��ġ, ȸ���� ���� ����Ǿ��� ���
            const bool reverse_z = m_Renderer ? m_Renderer->GetOption(Renderer::Option::ReverseZ) : false;
            if (m_Transform->HasPositionChangedThisFrame() || m_Transform->HasRotationChangedThisFrame() || m_PrevReverseZ != reverse_z)
            {
                m_PrevReverseZ = reverse_z;
                m_IsDirty = true;
            }

            // ���⼺ ������ ��� Cascade Shadow map ����� �ǽ��ؾ��Ѵ�.
            if (m_LightType == LightType::Directional)
            {
                if (shared_ptr<Camera> camera = m_Renderer->GetCamera())
                {
                    // ���� �������� �� ��İ� ����
                    if (m_PrevCameraView != camera->GetViewMatrix())
                    {
                        // ���Ӱ� �����Ѵ�.
                        m_PrevCameraView = camera->GetViewMatrix();
                        m_IsDirty = true;
                    }
                }
            }
        }

        // ��Ƽ���� �ʴٸ� �׳� ��ȯ
        if (!m_IsDirty)
            return;

        // ���⼺ ������ ���
        if (m_LightType == LightType::Directional)
        {
            // �Ÿ��� ���Ѵ�.
            float distance = m_Renderer->GetCamera() ? m_Renderer->GetCamera()->GetFarPlane() : 1000.0f;
            // �� �Ÿ���ŭ �ڷ� �̵���Ų��.
            m_Transform->SetPosition(-m_Transform->GetForward() * distance);
        }

        // �׸��� �� ������Ʈ
        if (m_ShadowsEnabled)
        {
            // ���� ���⼺ ������ ��� Cascade shadow map ����� �̿��Ѵ�.
            if (m_LightType == LightType::Directional)
            {
                ComputeCascadeSplits();
            }

            // �� ���
            ComputeViewMatrix();

            // �׸��� ��� �ؽ��� ���� ���� ��쿡 ����Ѵ�.
            if (m_ShadowMap.texture_depth)
            {
                for (uint32_t i = 0; i < m_ShadowMap.texture_depth->GetArrayLength(); i++)
                {
                    ComputeProjectionMatrix(i);
                }
            }
        }

        m_IsDirty = false;
    }

    void Light::Serialize(FileStream* stream)
    {
        // ����

        stream->Write(static_cast<uint32_t>(m_LightType));
        stream->Write(m_ShadowsEnabled);
        stream->Write(m_Shadows_screen_space_enabled);
        stream->Write(m_Shadows_transparent_enabled);
        stream->Write(m_VolumetricEnabled);
        stream->Write(m_ColorRGB);
        stream->Write(m_Range);
        stream->Write(m_Intensity);
        stream->Write(m_AngleRadian);
        stream->Write(m_Bias);
        stream->Write(m_NormalBias);
    }

    void Light::Deserialize(FileStream* stream)
    {
        // �ҷ�����

        SetLightType(static_cast<LightType>(stream->ReadAs<uint32_t>()));
        stream->Read(&m_ShadowsEnabled);
        stream->Read(&m_Shadows_screen_space_enabled);
        stream->Read(&m_Shadows_transparent_enabled);
        stream->Read(&m_VolumetricEnabled);
        stream->Read(&m_ColorRGB);
        stream->Read(&m_Range);
        stream->Read(&m_Intensity);
        stream->Read(&m_AngleRadian);
        stream->Read(&m_Bias);
        stream->Read(&m_NormalBias);
    }

    void Light::SetLightType(LightType type)
    {
        if (m_LightType == type)
            return;

        m_LightType = type;
        m_IsDirty = true;

        if (m_ShadowsEnabled)
        {
            CreateShadowMap();
        }

        m_Context->GetSubModule<World>()->Resolve();
    }


    void Light::SetShadowsEnabled(bool cast_shadows)
    {
        if (m_ShadowsEnabled == cast_shadows)
            return;

        m_ShadowsEnabled = cast_shadows;
        m_IsDirty = true;

        CreateShadowMap();
    }

    void Light::SetShadowsTransparentEnabled(bool cast_transparent_shadows)
    {
        if (m_Shadows_transparent_enabled == cast_transparent_shadows)
            return;

        m_Shadows_transparent_enabled = cast_transparent_shadows;
        m_IsDirty = true;

        CreateShadowMap();
    }

    void Light::SetRange(float range)
    {
        // ������ ���� ����
        m_Range = Util::Clamp(0.0f, std::numeric_limits<float>::max(), range);
        m_IsDirty = true;
    }

    void Light::SetAngle(float angle)
    {
        // ������ ���� ����
        m_AngleRadian = Util::Clamp(0.0f, Math::Util::PI_2, angle);
        m_IsDirty = true;
    }

    void Light::ComputeViewMatrix()
    {
        // ���⼺ ������ ���
        if (m_LightType == LightType::Directional)
        {
            // Cascade shadow mapping�� �ǽ��Ѵ�.
            if (!m_ShadowMap.slices.empty())
            {
                // ������ŭ �ݺ��ϸ鼭
                for (uint32_t i = 0; i < m_CascadeCount; i++)
                {
                    // �׸��� ���� �����´�.
                    ShadowSlice& shadow_map = m_ShadowMap.slices[i];
                    // ��ġ�� ����Ѵ�.
                    Vector3 position = shadow_map.center - m_Transform->GetForward() * shadow_map.max.z;
                    Vector3 target = shadow_map.center;
                    Vector3 up = Vector3::Up;
                    // ��� ����
                    m_MatrixViews[i] = Matrix::CreateLookAtLH(position, target, up);
                }
            }
        }
        else if (m_LightType == LightType::Spot)
        {
            const Vector3 position = m_Transform->GetPosition();
            const Vector3 forward = m_Transform->GetForward();
            const Vector3 up = m_Transform->GetUp();

            // ��� ����
            m_MatrixViews[0] = Matrix::CreateLookAtLH(position, position + forward, up);
        }
        else if (m_LightType == LightType::Point)
        {
            const Vector3 position = m_Transform->GetPosition();

            // ����Ʈ ����Ʈ�� ��� 6�� ��� ������ �� ����� �����Ѵ�.
            m_MatrixViews[0] = Matrix::CreateLookAtLH(position, position + Vector3::Right, Vector3::Up);       // x+
            m_MatrixViews[1] = Matrix::CreateLookAtLH(position, position + Vector3::Left, Vector3::Up);       // x-
            m_MatrixViews[2] = Matrix::CreateLookAtLH(position, position + Vector3::Up, Vector3::Backward); // y+
            m_MatrixViews[3] = Matrix::CreateLookAtLH(position, position + Vector3::Down, Vector3::Forward);  // y-
            m_MatrixViews[4] = Matrix::CreateLookAtLH(position, position + Vector3::Forward, Vector3::Up);       // z+
            m_MatrixViews[5] = Matrix::CreateLookAtLH(position, position + Vector3::Backward, Vector3::Up);       // z-
        }
    }

    bool Light::ComputeProjectionMatrix(uint32_t index /*= 0*/)
    {
        ASSERT(index < m_ShadowMap.texture_depth->GetArrayLength());

        ShadowSlice& shadow_slice = m_ShadowMap.slices[index];
        const bool reverse_z = m_Renderer ? m_Renderer->GetOption(Renderer::Option::ReverseZ) : false;
        
        // ���⼺ ������ ���
        if (m_LightType == LightType::Directional)
        {
            // ���̸� ����Ѵ�.
            const float cascade_depth = (shadow_slice.max.z - shadow_slice.min.z) * 10.0f;
            const float min_z = reverse_z ? cascade_depth : 0.0f;
            const float max_z = reverse_z ? 0.0f : cascade_depth;
            // ���� ������ ������.
            m_MatrixProjections[index] = Matrix::CreateOrthoOffCenterLH(shadow_slice.min.x, shadow_slice.max.x, shadow_slice.min.y, shadow_slice.max.y, min_z, max_z);
            shadow_slice.frustum = Frustum(m_MatrixViews[index], m_MatrixProjections[index], max_z);
        }
        // ����Ʈ ����Ʈ�̰ų� ���� ����Ʈ�� ���
        else
        {
            const uint32_t width = m_ShadowMap.texture_depth->GetWidth();
            const uint32_t height = m_ShadowMap.texture_depth->GetHeight();
            // ������ ����Ѵ�.
            const float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
            // ���� ����Ʈ�� ��� ������ �����Ѵ�.
            const float fov = m_LightType == LightType::Spot ? m_AngleRadian * 2.0f : Math::Util::PI_DIV_2;
            const float near_plane = reverse_z ? m_Range : 0.3f;
            const float far_plane = reverse_z ? 0.3f : m_Range;
            // ���� ��������� ������.
            m_MatrixProjections[index] = Matrix::CreatePerspectiveFieldOfViewLH(fov, aspect_ratio, near_plane, far_plane);
            shadow_slice.frustum = Frustum(m_MatrixViews[index], m_MatrixProjections[0], far_plane);
        }

        return true;
    }

    const Matrix& Light::GetViewMatrix(uint32_t index /*= 0*/) const
    {
        ASSERT(index < static_cast<uint32_t>(m_MatrixViews.size()));

        return m_MatrixViews[index];
    }

    const Matrix& Light::GetProjectionMatrix(uint32_t index /*= 0*/) const
    {
        ASSERT(index < static_cast<uint32_t>(m_MatrixProjections.size()));

        return m_MatrixProjections[index];
    }

    void Light::ComputeCascadeSplits()
    {
        if (m_ShadowMap.slices.empty())
            return;

        // ī�޶� ���� �������� �ʴ´ٸ� �׳� ��ȯ�Ѵ�.
        if (!m_Renderer->GetCamera())
            return;

        // ī�޶��� ����
        Camera* camera = m_Renderer->GetCamera().get();
        const float clip_near = camera->GetNearPlane();
        const float clip_far = camera->GetFarPlane();
        const Matrix view_projection_inverted = Matrix::Invert(camera->GetViewMatrix() * camera->ComputeProjection(false, clip_near, clip_far));

        // ī�޶��� ���������� �������� ������. https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        const float split_lambda = 0.98f;
        const float clip_range = clip_far - clip_near;
        const float min_z = clip_near;
        const float max_z = clip_near + clip_range;
        const float range = max_z - min_z;
        const float ratio = max_z / min_z;
        vector<float> splits(m_CascadeCount);
        for (uint32_t i = 0; i < m_CascadeCount; i++)
        {
            const float p = (i + 1) / static_cast<float>(m_CascadeCount);
            const float log = min_z * Math::Util::Pow(ratio, p);
            const float uniform = min_z + range * p;
            const float d = split_lambda * (log - uniform) + uniform;
            splits[i] = (d - clip_near) / clip_range;
        }

        for (uint32_t i = 0; i < m_CascadeCount; i++)
        {
            // Ŭ�� ������ �ڳ� �������� ���������� �ڳʸ� ������.
            Vector3 frustum_corners[8] =
            {
                Vector3(-1.0f,  1.0f, -1.0f),
                Vector3(1.0f,  1.0f, -1.0f),
                Vector3(1.0f, -1.0f, -1.0f),
                Vector3(-1.0f, -1.0f, -1.0f),
                Vector3(-1.0f,  1.0f,  1.0f),
                Vector3(1.0f,  1.0f,  1.0f),
                Vector3(1.0f, -1.0f,  1.0f),
                Vector3(-1.0f, -1.0f,  1.0f)
            };

            // ������ ������� �ڳʸ� ���忡 ��ġ
            for (Vector3& frustum_corner : frustum_corners)
            {
                // ��, ���������� ���� ���ؼ� ���忡 ��ġ��Ų��.
                Vector4 inverted_corner = Vector4(frustum_corner, 1.0f) * view_projection_inverted;
                // Perspective division
                frustum_corner = inverted_corner / inverted_corner.w;
            }

            // ���ø� ���Ͻ��� ���Ѵ�.
            {
                // �ٽ� �����Ҷ����� �ʱ�ȭ�Ѵ�.
                static float split_distance_previous;
                if (i == 0) 
                    split_distance_previous = 0.0f;

                const float split_distance = splits[i];
                // �Ÿ��� ���Ѵ�.
                for (uint32_t i = 0; i < 4; i++)
                {
                    Vector3 distance = frustum_corners[i + 4] - frustum_corners[i];
                    frustum_corners[i + 4] = frustum_corners[i] + (distance * split_distance);
                    frustum_corners[i] = frustum_corners[i] + (distance * split_distance_previous);
                }
                split_distance_previous = splits[i];
            }

            // ���������� �ٿ�� ������ ���Ѵ�.
            {
                ShadowSlice& shadow_slice = m_ShadowMap.slices[i];

                // �߽� ��ġ�� ���Ѵ�.
                shadow_slice.center = Vector3::Zero;
                for (const Vector3& frustum_corner : frustum_corners)
                {
                    shadow_slice.center += Vector3(frustum_corner);
                }

                // 8�� ���� �߽� ��ġ ���
                shadow_slice.center /= 8.0f;

                // ������ ���
                float radius = 0.0f;
                for (const Vector3& frustum_corner : frustum_corners)
                {
                    const float distance = Vector3::Distance(frustum_corner, shadow_slice.center);
                    radius = Util::Max(radius, distance);
                }
                radius = Util::Ceil(radius * 16.0f) / 16.0f;

                // �ִ�, �ּ�ġ ���
                shadow_slice.max = radius;
                shadow_slice.min = -radius;
            }
        }
    }

    uint32_t Light::GetShadowArraySize() const
    {
        return m_ShadowMap.texture_depth ? m_ShadowMap.texture_depth->GetArrayLength() : 0;
    }

    void Light::CreateShadowMap()
    {
        // ���� �ػ��� ��ȭ�� ���ٸ� �׳� ��ȯ�Ѵ�.
        const uint32_t resolution = m_Renderer->GetOptionValue<uint32_t>(Renderer::OptionValue::ShadowResolution);
        const bool resolution_changed = m_ShadowMap.texture_depth ? (resolution != m_ShadowMap.texture_depth->GetWidth()) : false;
        if ((!m_IsDirty && !resolution_changed))
            return;

        // �׸��ڸ� �������� �ʴ� ������ ��쵵 �׳� ��ȯ�Ѵ�.
        if (!m_ShadowsEnabled)
        {
            m_ShadowMap.texture_depth.reset();
            return;
        }

        if (!m_Shadows_transparent_enabled)
        {
            m_ShadowMap.texture_color.reset();
        }

        // ���⼺ ������ ��� ��� �ؽ��ĸ� ������.
        if (GetLightType() == LightType::Directional)
        {
            m_ShadowMap.texture_depth = make_unique<RHI_Texture2DArray>(m_Context, resolution, resolution, RHI_Format_D32_Float, m_CascadeCount, RHI_Texture_Rt_DepthStencil | RHI_Texture_Srv, "shadow_map_directional");

            if (m_Shadows_transparent_enabled)
            {
                m_ShadowMap.texture_color = make_unique<RHI_Texture2DArray>(m_Context, resolution, resolution, RHI_Format_R8G8B8A8_Unorm, m_CascadeCount, RHI_Texture_Rt_Color | RHI_Texture_Srv, "shadow_map_directional_color");
            }

            // Cascade shadow map�� ���ؼ� �����̽��� ������.
            m_ShadowMap.slices = vector<ShadowSlice>(m_CascadeCount);
        }
        // ����Ʈ ����Ʈ�� ��� ť�� �ؽ��ĸ��� �̿��Ѵ�.
        else if (GetLightType() == LightType::Point)
        {
            m_ShadowMap.texture_depth = make_unique<RHI_TextureCube>(m_Context, resolution, resolution, RHI_Format_D32_Float, RHI_Texture_Rt_DepthStencil | RHI_Texture_Srv, "shadow_map_point_color");

            if (m_Shadows_transparent_enabled)
            {
                m_ShadowMap.texture_color = make_unique<RHI_TextureCube>(m_Context, resolution, resolution, RHI_Format_R8G8B8A8_Unorm, RHI_Texture_Rt_Color | RHI_Texture_Srv, "shadow_map_point_color");
            }

            // 6�� �̿�
            m_ShadowMap.slices = vector<ShadowSlice>(6);
        }
        // ���� ����Ʈ�� ��� �Ϲ� �ؽ��� �̿�
        else if (GetLightType() == LightType::Spot)
        {
            m_ShadowMap.texture_depth = make_unique<RHI_Texture2D>(m_Context, resolution, resolution, 1, RHI_Format_D32_Float, RHI_Texture_Rt_DepthStencil | RHI_Texture_Srv, "shadow_map_spot_color");

            if (m_Shadows_transparent_enabled)
            {
                m_ShadowMap.texture_color = make_unique<RHI_Texture2D>(m_Context, resolution, resolution, 1, RHI_Format_R8G8B8A8_Unorm, RHI_Texture_Rt_Color | RHI_Texture_Srv, "shadow_map_spot_color");
            }

            // 1�� �̿�
            m_ShadowMap.slices = vector<ShadowSlice>(1);
        }
    }

    bool Light::IsInViewFrustum(Renderable* renderable, uint32_t index) const
    {
        // �������� ��ü�� �������� �ȿ� �����ϴ��� Ȯ���Ѵ�.
        const auto box = renderable->GetAabb();
        const auto center = box.GetCenter();
        const auto extents = box.GetExtents();

        // ���� ���⼺ ������ ��� �÷��̾��� ������ �ʴ� ������ �׸��ڸ� ������ ���� ������ �� ���� ����ؼ� near plane�� �����ϰ� �����Ѵ�.
        const bool ignore_near_plane = (m_LightType == LightType::Directional) ? true : false;

        return m_ShadowMap.slices[index].frustum.IsVisible(center, extents, ignore_near_plane);
    }
}