#pragma once

#include "IComponent.h"
#include "../../Math/Vector3.h"
#include "../../Math/Vector2.h"
#include "../../Math/Quaternion.h"

class btTypedConstraint;

namespace PlayGround
{
    class RigidBody;
    class Entity;
    class Physics;

    // 컨스트레인트 타입
    enum ConstraintType
    {
        ConstraintType_Point,
        ConstraintType_Hinge,
        ConstraintType_Slider,
        ConstraintType_ConeTwist
    };

    // 컨스트레인트 컴포넌트
    class Constraint : public IComponent
    {
    public:
        Constraint(Context* context, Entity* entity, uint64_t id = 0);
        ~Constraint();

        // IComponent 가상 메서드 오버라이드
        void OnInit() override;
        void OnStart() override;
        void OnStop() override;
        void OnRemove() override;
        void Update(double delta_time) override;
        void Serialize(FileStream* stream) override;
        void Deserialize(FileStream* stream) override;

        inline ConstraintType GetConstraintType() const { return m_ConstraintType; }
        void SetConstraintType(ConstraintType type);

        inline const Math::Vector2& GetHighLimit() const { return m_HighLimit; }
        void SetHighLimit(const Math::Vector2& limit);

        inline const Math::Vector2& GetLowLimit() const { return m_LowLimit; }
        void SetLowLimit(const Math::Vector2& limit);

        inline const Math::Vector3& GetPosition() const { return m_Position; }
        void SetPosition(const Math::Vector3& position);

        inline const Math::Quaternion& GetRotation() const { return m_Rotation; }
        void SetRotation(const Math::Quaternion& rotation);

        inline const Math::Vector3& GetPositionOther() const { return m_PositionOther; }
        void SetPositionOther(const Math::Vector3& position);

        inline const Math::Quaternion& GetRotationOther() const { return m_RotationOther; }
        void SetRotationOther(const Math::Quaternion& rotation);

        inline std::weak_ptr<Entity> GetBodyOther() const { return m_BodyOther; }
        void SetBodyOther(const std::weak_ptr<Entity>& body_other);

        void ReleaseConstraint();
        void ApplyFrames() const;

    private:
        void Construct();
        void ApplyLimits() const;

        btTypedConstraint* m_Constraint;

        ConstraintType m_ConstraintType;
        Math::Vector3 m_Position;
        Math::Quaternion m_Rotation;
        Math::Vector2 m_HighLimit;
        Math::Vector2 m_LowLimit;

        std::weak_ptr<Entity> m_BodyOther;
        Math::Vector3 m_PositionOther;
        Math::Quaternion m_RotationOther;

        float m_ErrorReduction;
        float m_ConstraintForceMixing;
        bool m_EnabledEffective;
        bool m_CollisionWithLinkedBody;
        bool m_DeferredConstruction;

        Physics* m_Physics;
    };
}