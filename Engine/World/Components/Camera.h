#pragma once

#include "IComponent.h"
#include <memory>
#include "../../RHI/RHI_Definition.h"
#include "../../RHI/RHI_Viewport.h"
#include "../../Math/Matrix.h"
#include "../../Math/Ray.h"
#include "../../Math/Frustum.h"
#include "../../Math/Vector2.h"
#include "../../Math/Rectangle.h"


namespace PlayGround
{
    class Entity;
    class Model;
    class Renderable;
    class Input;
    class Renderer;

    enum ProjectionType
    {
        Projection_Perspective,
        Projection_Orthographic,
    };

    // 카메라 컴포넌트
    class Camera : public IComponent
    {
    public:
        Camera(Context* context, Entity* entity, uint64_t id = 0);
        ~Camera() = default;

        void OnInit() override;
        void Update(double delta_time) override;
        void Serialize(FileStream* stream) override;
        void Deserialize(FileStream* stream) override;

        inline const Math::Matrix& GetViewMatrix()           const { return m_view; }
        inline const Math::Matrix& GetProjectionMatrix()     const { return m_projection; }
        inline const Math::Matrix& GetViewProjectionMatrix() const { return m_view_projection; }

        inline const Math::Ray& GetPickingRay() const { return m_ray; }

        bool Pick(std::shared_ptr<Entity>& entity);

        Math::Vector2 WorldToScreenCoordinates(const Math::Vector3& position_world) const;

        Math::Rectangle WorldToScreenCoordinates(const Math::BoundingBox& bounding_box) const;

        Math::Vector3 ScreenToWorldCoordinates(const Math::Vector2& position_screen, const float z) const;

        inline float GetAperture() const { return m_aperture; }
        inline void SetAperture(const float aperture) { m_aperture = aperture; }

        inline float GetShutterSpeed() const { return m_shutter_speed; }
        inline void SetShutterSpeed(const float shutter_speed) { m_shutter_speed = shutter_speed; }

        inline float GetIso() const { return m_iso; }
        inline void SetIso(const float iso) { m_iso = iso; }

        // https://google.github.io/filament/Filament.md.html#lighting/units/lightunitsvalidation
        inline float GetEv100()    const { return std::log2((m_aperture * m_aperture) / m_shutter_speed * 100.0f / m_iso); } 
        // https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
        inline float GetExposure() const { return 1.0f / (std::pow(2.0f, GetEv100()) * 1.2f); }

        void SetNearPlane(float near_plane);
        void SetFarPlane(float far_plane);
        void SetProjection(ProjectionType projection);
        inline float GetNearPlane()               const { return m_near_plane; }
        inline float GetFarPlane()                const { return m_far_plane; }
        inline ProjectionType GetProjectionType() const { return m_projection_type; }

        float GetFovHorizontalRad() const { return m_fov_horizontal_rad; }
        float GetFovVerticalRad()   const;
        float GetFovHorizontalDeg() const;
        void SetFovHorizontalDeg(float fov);
        const RHI_Viewport& GetViewport() const;

        bool IsInViewFrustum(Renderable* renderable) const;
        bool IsInViewFrustum(const Math::Vector3& center, const Math::Vector3& extents) const;

        inline const Math::Vector4& GetClearColor() const { return m_clear_color; }
        inline void SetClearColor(const Math::Vector4& color) { m_clear_color = color; }
        inline bool GetFpsControlEnabled()                    const { return m_fps_control_enabled; }
        inline void SetFpsControlEnabled(const bool enabled) { m_fps_control_enabled = enabled; }
        inline bool IsFpsControlled()                         const { return m_fps_control_assumed; }
        void MakeDirty() { m_is_dirty = true; }

        Math::Matrix ComputeViewMatrix() const;
        Math::Matrix ComputeProjection(const bool reverse_z, const float near_plane = 0.0f, const float far_plane = 0.0f);

    private:
        void ProcessInput(double delta_time);
        void ProcessInputFpsControl(double delta_time);
        void ProcessInputLerpToEntity(double delta_time);

        float m_aperture = 50.0f;       
        float m_shutter_speed = 1.0f / 60.0f;
        float m_iso = 500.0f;
        float m_fov_horizontal_rad = Math::Util::DegreesToRadians(90.0f);
        float m_near_plane = 0.3f;
        float m_far_plane = 1000.0f;
        ProjectionType m_projection_type = Projection_Perspective;
        Math::Vector4 m_clear_color = Math::Vector4(0.396f, 0.611f, 0.937f, 1.0f); 
        Math::Matrix m_view = Math::Matrix::Identity;
        Math::Matrix m_projection = Math::Matrix::Identity;
        Math::Matrix m_view_projection = Math::Matrix::Identity;
        Math::Vector3 m_position = Math::Vector3::Zero;
        Math::Quaternion m_rotation = Math::Quaternion::Identity;
        bool m_is_dirty = false;
        bool m_fps_control_enabled = true;
        bool m_fps_control_assumed = false;
        Math::Vector2 m_mouse_last_position = Math::Vector2::Zero;
        bool m_fps_control_cursor_hidden = false;
        Math::Vector3 m_movement_speed = Math::Vector3::Zero;
        float m_movement_speed_min = 0.5f;
        float m_movement_speed_max = 5.0f;
        float m_movement_acceleration = 1000.0f;
        float m_movement_drag = 10.0f;
        Math::Vector2 m_mouse_smoothed = Math::Vector2::Zero;
        Math::Vector2 m_mouse_rotation = Math::Vector2::Zero;
        float m_mouse_sensitivity = 0.2f;
        float m_mouse_smoothing = 0.5f;
        bool m_lerp_to_target = false;
        float m_lerp_to_target_alpha = 0.0f;
        float m_lerp_to_target_speed = 0.0f;
        Math::Vector3 m_lerp_to_target_position = Math::Vector3::Zero;
        RHI_Viewport m_last_known_viewport;
        Math::Ray m_ray;
        Math::Frustum m_frustum;

        Renderer* m_renderer = nullptr;
        Input* m_input = nullptr;
    };
}
