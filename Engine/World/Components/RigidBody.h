#pragma once

#include "IComponent.h"
#include <vector>
#include "../../Math/Vector3.h"

class btRigidBody;
class btCollisionShape;


namespace PlayGround
{
    class Entity;
    class Constraint;
    class Physics;
    namespace Math { class Quaternion; }

    // 포스 모드
    enum ForceMode
    {
        Force,
        Impulse
    };

    // 강체 물리
    class RigidBody : public IComponent
    {
    public:
        RigidBody(Context* context, Entity* entity, uint64_t id = 0);
        ~RigidBody();

        // IComponent 가상 메서드 오버라이드
        void OnInit() override;
        void OnRemove() override;
        void OnStart() override;
        void OnStop() override;
        void Update(double delta_time) override;
        void Serialize(FileStream* stream) override;
        void Deserialize(FileStream* stream) override;

        inline float GetMass() const { return m_Mass; }
        void SetMass(float mass);

        inline float GetFriction() const { return m_Friction; }
        void SetFriction(float friction);

        inline float GetFrictionRolling() const { return m_FrictionRolling; }
        void SetFrictionRolling(float frictionRolling);

        inline float GetRestitution() const { return m_Restitution; }
        void SetRestitution(float restitution);

        void SetUseGravity(bool gravity);
        inline bool GetUseGravity() const { return m_UseGravity; }
        inline Math::Vector3 GetGravity() const { return m_Gravity; }
        void SetGravity(const Math::Vector3& acceleration);

        void SetIsKinematic(bool kinematic);
        inline bool GetIsKinematic() const { return m_IsKinematic; }

        void SetLinearVelocity(const Math::Vector3& velocity, const bool activate = true) const;
        void SetAngularVelocity(const Math::Vector3& velocity, const bool activate = true) const;
        void ApplyForce(const Math::Vector3& force, ForceMode mode) const;
        void ApplyForceAtPosition(const Math::Vector3& force, const Math::Vector3& position, ForceMode mode) const;
        void ApplyTorque(const Math::Vector3& torque, ForceMode mode) const;

        void SetPositionLock(bool lock);
        void SetPositionLock(const Math::Vector3& lock);
        inline Math::Vector3 GetPositionLock() const { return m_PositionLock; }

        void SetRotationLock(bool lock);
        void SetRotationLock(const Math::Vector3& lock);
        inline Math::Vector3 GetRotationLock() const { return m_RotationLock; }

        void SetCenterOfMass(const Math::Vector3& centerOfMass);
        inline const Math::Vector3& GetCenterOfMass() const { return m_CenterOfMass; }

        Math::Vector3 GetPosition() const;
        void SetPosition(const Math::Vector3& position, const bool activate = true) const;

        Math::Quaternion GetRotation() const;
        void SetRotation(const Math::Quaternion& rotation, const bool activate = true) const;

        void ClearForces() const;
        void Activate() const;
        void Deactivate() const;
        inline btRigidBody* GetBtRigidBody() const { return m_RigidBody; }
        inline bool IsInWorld() const { return m_InWorld; }

        void AddConstraint(Constraint* constraint);
        void RemoveConstraint(Constraint* constraint);
        void SetShape(btCollisionShape* shape);
    private:
        void BodyAddToWorld();
        void BodyRelease();
        void BodyRemoveFromWorld();
        void BodyAcquireShape();
        void FlagsUpdateKinematic() const;
        void FlagsUpdateGravity() const;
        bool IsActivated() const;

        float m_Mass = 1.0f;
        float m_Friction = 0.0f;
        float m_FrictionRolling = 0.0f;
        float m_Restitution = 0.0f;
        bool m_UseGravity = false;
        bool m_IsKinematic = false;
        Math::Vector3 m_Gravity = Math::Vector3::Zero;
        Math::Vector3 m_PositionLock = Math::Vector3::Zero;
        Math::Vector3 m_RotationLock = Math::Vector3::Zero;
        Math::Vector3 m_CenterOfMass = Math::Vector3::Zero;

        btRigidBody* m_RigidBody = nullptr;
        btCollisionShape* m_CollisionShape = nullptr;
        bool m_InWorld = false;
        Physics* m_Physics = nullptr;
        std::vector<Constraint*> m_vecConstraints;
    };
}
