#pragma once

#include "IComponent.h"
#include "../../Math/Vector3.h"

class btCollisionShape;

namespace PlayGround
{
	class Mesh;

    // 충돌체 타입
    enum ColliderShape
    {
        ColliderShape_Box,
        ColliderShape_Sphere,
        ColliderShape_StaticPlane,
        ColliderShape_Cylinder,
        ColliderShape_Capsule,
        ColliderShape_Cone,
        ColliderShape_Mesh,
    };

    // 충돌체 컴포넌트
	class Collider : public IComponent
	{
    public:
        Collider(Context* context, Entity* entity, uint64_t id = 0);
        ~Collider() = default;

        // IComponent 가상 메서드 오버라이드
        void OnInit() override;
        void OnRemove() override;
        void Serialize(FileStream* strema) override;
        void Deserialize(FileStream* stream) override;

        inline const Math::Vector3& GetBoundingBox() const { return m_Size; }

        void SetBoundingBox(const Math::Vector3& boundingBox);

        inline const Math::Vector3& GetCenter() const { return m_Center; }

        void SetCenter(const Math::Vector3& center);

        inline ColliderShape GetShapeType() const { return m_ShapeType; }

        void SetShapeType(ColliderShape type);

        inline const auto& GetShape() { return m_Shape; }

        inline bool GetOptimize() const { return m_Optimize; }

        void SetOptimize(bool optimize);

    private:
        void ShapeUpdate();
        void ShapeRelease();
        void RigidBodySetShape(btCollisionShape* shape) const;
        void RigidBodySetCenterOfMass(const Math::Vector3& center) const;

        ColliderShape m_ShapeType;
        btCollisionShape* m_Shape;
        Math::Vector3 m_Size;
        Math::Vector3 m_Center;
        uint32_t m_VertexLimit = 100000;
        bool m_Optimize = true;
	};
}