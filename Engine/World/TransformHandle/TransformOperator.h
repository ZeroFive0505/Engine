#pragma once

#include "TransformEnums.h"
#include "TransformOperatorAxis.h"
#include <memory>
#include "../../Rendering/Model.h"

namespace PlayGround
{
    class Renderer;
    class Context;
    class RHI_VertexBuffer;
    class RHI_IndexBuffer;
    class Entity;
    class Input;
    class Camera;
    class Model;
    class Transform;

    namespace Math
    {
        class Ray;
    }

	class TransformOperator
	{
    public:
        TransformOperator(Context* context, const TransformHandleType transform_handle_type);
        ~TransformOperator() = default;

        void Update(TransformHandleSpace space, Entity* entity, Camera* camera, float handle_size);
        const Math::Matrix& GetTransform(const Math::Vector3& axis) const;
        const Math::Vector3& GetColor(const Math::Vector3& axis) const;
        const RHI_VertexBuffer* GetVertexBuffer();
        const RHI_IndexBuffer* GetIndexBuffer();
        inline bool HasModel() const { return m_AxisModel != nullptr; }
        bool IsEditing() const;
        bool IsHovered() const;

    private:
        void SnapToTransform(TransformHandleSpace space, Entity* entity, Camera* camera, float handle_size);

    protected:
        // Test if the mouse ray intersects any of the handles.
        virtual void InteresectionTest(const Math::Ray& mouse_ray) = 0;
        // Compute transformation (position, rotation or scale) delta.
        virtual void ComputeDelta(const Math::Ray& mouse_ray, const Camera* camera) = 0;
        // Map the transformation delta to the entity's transform.
        virtual void MapToTransform(Transform* transform, const TransformHandleSpace space) = 0;

        TransformOperatorAxis m_Handle_x;
        TransformOperatorAxis m_Handle_y;
        TransformOperatorAxis m_Handle_z;
        TransformOperatorAxis m_Handle_xyz;

        bool m_Handle_x_intersected = false;
        bool m_Handle_y_intersected = false;
        bool m_Handle_z_intersected = false;
        bool m_Handle_xyz_intersected = false;

        TransformHandleType m_Type = TransformHandleType::Unknown;

        bool m_Offset_handle_axes_from_center = true;
        float m_Offset_handle_from_center = 0.0f;

        Math::Vector3 m_Position = Math::Vector3::Zero;
        Math::Vector3 m_Rotation = Math::Vector3::Zero;
        Math::Vector3 m_Scale = Math::Vector3::Zero;

        Context* m_Context = nullptr;
        Renderer* m_Renderer = nullptr;
        Input* m_Input = nullptr;
        std::unique_ptr<Model> m_AxisModel;
	};
}


