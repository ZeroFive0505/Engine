#include "Common.h"
#include "RigidBody.h"
#include "Transform.h"
#include "Collider.h"
#include "Constraint.h"
#include "../Entity.h"
#include "../../Physics/Physics.h"
#include "../../Physics/BulletPhysicsHelper.h"
#include "../../IO/FileStream.h"
#include "../../Core/Context.h"
#include "../../Core/Engine.h"

#include "LinearMath/btMotionState.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
    static const float DEFAULT_MASS = 1.0f;
    static const float DEFAULT_FRICTION = 0.5f;
    static const float DEFAULT_FRICTION_ROLLING = 0.0f;
    static const float DEFAULT_RESTITUTION = 0.0f;
    static const float DEFAULT_DEACTIVATION_TIME = 2000;

    // ���� ���� ��� ���� btMotionState�� ��ӹ޴´�.
    class MotionState : public btMotionState
    {
    public:
        MotionState(RigidBody* rigidBody) : m_RigidBody(rigidBody) {}

        void getWorldTransform(btTransform& worldTrans) const override
        {
            // ����� btTransform������ �������� ���Ǵ� �����ͷ� ����
            const Vector3 lastPos = m_RigidBody->GetTransform()->GetPosition();
            const Quaternion lastRot = m_RigidBody->GetTransform()->GetRotation();

            // �߽����� ȸ��ġ ����
            worldTrans.setOrigin(ToBtVector3(lastPos + lastRot * m_RigidBody->GetCenterOfMass()));
            worldTrans.setRotation(ToBtQuaternion(lastRot));
        }

        void setWorldTransform(const btTransform& worldTrans) override
        {
            // ����� ��������� �� ��ü�� ��ġ, ȸ���� ���Ѵ�.
            const Quaternion newWorldRot = ToQuaternion(worldTrans.getRotation());
            const Vector3 newWorldPos = ToVector3(worldTrans.getOrigin()) - newWorldRot * m_RigidBody->GetCenterOfMass();

            m_RigidBody->GetTransform()->SetPosition(newWorldPos);
            m_RigidBody->GetTransform()->SetRotation(newWorldRot);
        }
    private:
        RigidBody* m_RigidBody;
    };

    RigidBody::RigidBody(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
    {
        // ������ �ٵ� �ʱ�ȭ

        m_Physics = GetContext()->GetSubModule<Physics>();

        m_InWorld = false;
        m_Mass = DEFAULT_MASS;
        m_Restitution = DEFAULT_RESTITUTION;
        m_Friction = DEFAULT_FRICTION;
        m_FrictionRolling = DEFAULT_FRICTION_ROLLING;
        m_UseGravity = true;
        m_Gravity = m_Physics->GetGravity();
        m_IsKinematic = false;
        m_PositionLock = Vector3::Zero;
        m_RotationLock = Vector3::Zero;
        m_CollisionShape = nullptr;
        m_RigidBody = nullptr;

        // Getter, Setter ����
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Mass, float);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Friction, float);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_FrictionRolling, float);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Restitution, float);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_UseGravity, bool);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_IsKinematic, bool);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Gravity, Vector3);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_PositionLock, Vector3);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_RotationLock, Vector3);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_CenterOfMass, Vector3);
    }

    RigidBody::~RigidBody()
    {
        BodyRelease();
    }

    void RigidBody::OnInit()
    {
        BodyAcquireShape();
        BodyAddToWorld();
    }

    void RigidBody::OnRemove()
    {
        BodyRelease();
    }

    void RigidBody::OnStart()
    {
        Activate();
    }

    void RigidBody::OnStop()
    {
        // �����͸��� ���ͽ� ��� �����͸� �����·� �ǵ�����.
        SetPosition(GetTransform()->GetIninPosition(), false);
        SetRotation(GetTransform()->GetinitRotation(), false);
        SetLinearVelocity(Vector3::Zero, false);
        SetAngularVelocity(Vector3::Zero, false);
    }

    void RigidBody::Update(double delta_time)
    {
        // ���� ���Ӹ�尡 �ƴҰ�� ������ ���� ��ġ, ȸ�� ��ġ�� ������ �� �ְ��Ѵ�.
        if (!IsActivated() || !m_Context->m_Engine->IsEngineModeSet(GameMode))
        {
            if (GetPosition() != GetTransform()->GetPosition())
            {
                SetPosition(GetTransform()->GetPosition(), false);
                SetLinearVelocity(Vector3::Zero, false);
                SetAngularVelocity(Vector3::Zero, false);
            }

            if (GetRotation() != GetTransform()->GetRotation())
            {
                SetRotation(GetTransform()->GetRotation(), false);
                SetLinearVelocity(Vector3::Zero, false);
                SetAngularVelocity(Vector3::Zero, false);
            }
        }
    }

    void RigidBody::Serialize(FileStream* stream)
    {
        // ����
        stream->Write(m_Mass);
        stream->Write(m_Friction);
        stream->Write(m_FrictionRolling);
        stream->Write(m_Restitution);
        stream->Write(m_UseGravity);
        stream->Write(m_Gravity);
        stream->Write(m_IsKinematic);
        stream->Write(m_PositionLock);
        stream->Write(m_RotationLock);
        stream->Write(m_InWorld);
    }

    void RigidBody::Deserialize(FileStream* stream)
    {
        // �ҷ�����
        stream->Read(&m_Mass);
        stream->Read(&m_Friction);
        stream->Read(&m_FrictionRolling);
        stream->Read(&m_Restitution);
        stream->Read(&m_UseGravity);
        stream->Read(&m_Gravity);
        stream->Read(&m_IsKinematic);
        stream->Read(&m_PositionLock);
        stream->Read(&m_RotationLock);
        stream->Read(&m_InWorld);

        // �ҷ��� ������ �ٵ� �ʱ�ȭ
        BodyAcquireShape();
        BodyAddToWorld();
    }

    void RigidBody::SetMass(float mass)
    {
        // ���� ����
        mass = Util::Max(mass, 0.0001f);
        if (mass != m_Mass)
        {
            m_Mass = mass;
            BodyAddToWorld();
        }
    }

    void RigidBody::SetFriction(float friction)
    {
        // ���� ����
        if (!m_RigidBody || m_Friction == friction)
            return;

        m_Friction = friction;
        m_RigidBody->setFriction(friction);
    }

    void RigidBody::SetFrictionRolling(float frictionRolling)
    {
        // ���� ���� ����
        if (!m_RigidBody || m_FrictionRolling == frictionRolling)
            return;

        m_FrictionRolling = frictionRolling;
        m_RigidBody->setRollingFriction(frictionRolling);
    }

    void RigidBody::SetRestitution(float restitution)
    {
        // ������
        if (!m_RigidBody || m_Restitution == restitution)
            return;

        m_Restitution = restitution;
        m_RigidBody->setRestitution(restitution);
    }

    void RigidBody::SetUseGravity(bool gravity)
    {
        // ���
        if (gravity == m_UseGravity)
            return;

        m_UseGravity = gravity;
        BodyAddToWorld();
    }

    void RigidBody::SetGravity(const Vector3& acceleration)
    {
        // �߷� ��ġ
        if (m_Gravity == acceleration)
            return;

        m_Gravity = acceleration;
        BodyAddToWorld();
    }

    void RigidBody::SetIsKinematic(bool kinematic)
    {
        // Ű�׸�ƽ ����
        if (kinematic == m_IsKinematic)
            return;

        m_IsKinematic = kinematic;
        BodyAddToWorld();
    }

    void RigidBody::SetLinearVelocity(const Vector3& velocity, const bool activate /*= true*/) const
    {
        // �ӵ� ����
        if (!m_RigidBody)
            return;

        m_RigidBody->setLinearVelocity(ToBtVector3(velocity));
        if (velocity != Vector3::Zero && activate)
        {
            Activate();
        }
    }

    void RigidBody::SetAngularVelocity(const Vector3& velocity, const bool activate /*= true*/) const
    {
        // ���ӵ� ����
        if (!m_RigidBody)
            return;

        m_RigidBody->setAngularVelocity(ToBtVector3(velocity));
        if (velocity != Vector3::Zero && activate)
        {
            Activate();
        }
    }

    void RigidBody::ApplyForce(const Vector3& force, ForceMode mode) const
    {
        if (!m_RigidBody)
            return;

        Activate();

        // ������ �ٵ� ���� ���Ѵ�.

        if (mode == Force)
        {
            m_RigidBody->applyCentralForce(ToBtVector3(force));
        }
        else if (mode == Impulse)
        {
            m_RigidBody->applyCentralImpulse(ToBtVector3(force));
        }
    }

    void RigidBody::ApplyForceAtPosition(const Vector3& force, const Vector3& position, ForceMode mode) const
    {
        // Ư�� ��ġ���� ���� ���Ѵ�.
        if (!m_RigidBody)
            return;

        Activate();

        if (mode == Force)
        {
            m_RigidBody->applyForce(ToBtVector3(force), ToBtVector3(position));
        }
        else if (mode == Impulse)
        {
            m_RigidBody->applyImpulse(ToBtVector3(force), ToBtVector3(position));
        }
    }

    void RigidBody::ApplyTorque(const Vector3& torque, ForceMode mode) const
    {
        // ��ũ ����
        if (!m_RigidBody)
            return;

        Activate();

        if (mode == Force)
        {
            m_RigidBody->applyTorque(ToBtVector3(torque));
        }
        else if (mode == Impulse)
        {
            m_RigidBody->applyTorqueImpulse(ToBtVector3(torque));
        }
    }

    void RigidBody::SetPositionLock(bool lock)
    {
        // ��ġ ���� �Ǵ�.
        if (lock)
        {
            SetPositionLock(Vector3::One);
        }
        else
        {
            SetPositionLock(Vector3::Zero);
        }
    }

    void RigidBody::SetPositionLock(const Vector3& lock)
    {
        // ��ġ ���� �Ǵ�.
        if (!m_RigidBody || m_PositionLock == lock)
            return;

        m_PositionLock = lock;
        m_RigidBody->setLinearFactor(ToBtVector3(Vector3::One - lock));
    }

    void RigidBody::SetRotationLock(bool lock)
    {
        // ȸ�� ���� �Ǵ�.
        if (lock)
        {
            SetRotationLock(Vector3::One);
        }
        else
        {
            SetRotationLock(Vector3::Zero);
        }
    }

    void RigidBody::SetRotationLock(const Vector3& lock)
    {
        if (!m_RigidBody || m_RotationLock == lock)
            return;

        m_RotationLock = lock;
        m_RigidBody->setAngularFactor(ToBtVector3(Vector3::One - lock));
    }

    void RigidBody::SetCenterOfMass(const Vector3& centerOfMass)
    {
        // �߽����� ����Ѵ�.
        m_CenterOfMass = centerOfMass;
        SetPosition(GetPosition());
    }

    Vector3 RigidBody::GetPosition() const
    {
        // ��ġ ��ȯ
        if (m_RigidBody)
        {
            const btTransform& transform = m_RigidBody->getWorldTransform();
            return ToVector3(transform.getOrigin()) - ToQuaternion(transform.getRotation()) * m_CenterOfMass;
        }

        return Vector3::Zero;
    }

    void RigidBody::SetPosition(const Vector3& position, const bool activate /*= true*/) const
    {
        if (!m_RigidBody)
            return;

        btTransform& transform_world = m_RigidBody->getWorldTransform();
        transform_world.setOrigin(ToBtVector3(position + ToQuaternion(transform_world.getRotation()) * m_CenterOfMass));

        // ���������� Ʈ���������� ��ġ�� �����Ѵ�.
        btTransform transform_world_interpolated = m_RigidBody->getInterpolationWorldTransform();
        transform_world_interpolated.setOrigin(transform_world.getOrigin());
        m_RigidBody->setInterpolationWorldTransform(transform_world_interpolated);

        if (activate)
        {
            Activate();
        }
    }

    Quaternion RigidBody::GetRotation() const
    {
        return m_RigidBody ? ToQuaternion(m_RigidBody->getWorldTransform().getRotation()) : Quaternion::Identity;
    }

    void RigidBody::SetRotation(const Quaternion& rotation, const bool activate /*= true*/) const
    {
        if (!m_RigidBody)
            return;

        // ȸ�� ����
        const Vector3 oldPosition = GetPosition();
        btTransform& transform_world = m_RigidBody->getWorldTransform();
        transform_world.setRotation(ToBtQuaternion(rotation));
        // ���� �߽����� �������ٸ�
        if (m_CenterOfMass != Vector3::Zero)
        {
            transform_world.setOrigin(ToBtVector3(oldPosition + rotation * m_CenterOfMass));
        }

        // ���� ������ ȸ�� ��ġ ����
        btTransform interpTrans = m_RigidBody->getInterpolationWorldTransform();
        interpTrans.setRotation(transform_world.getRotation());
        // �߽����� �������ٸ�
        if (m_CenterOfMass != Vector3::Zero)
        {
            interpTrans.setOrigin(transform_world.getOrigin());
        }
        m_RigidBody->setInterpolationWorldTransform(interpTrans);

        m_RigidBody->updateInertiaTensor();

        if (activate)
        {
            Activate();
        }
    }

    void RigidBody::ClearForces() const
    {
        if (!m_RigidBody)
            return;

        m_RigidBody->clearForces();
    }

    void RigidBody::Activate() const
    {
        if (!m_RigidBody)
            return;

        if (m_Mass > 0.0f)
        {
            m_RigidBody->activate(true);
        }
    }

    void RigidBody::Deactivate() const
    {
        if (!m_RigidBody)
            return;

        m_RigidBody->setActivationState(WANTS_DEACTIVATION);
    }

    void RigidBody::AddConstraint(Constraint* constraint)
    {
        m_vecConstraints.emplace_back(constraint);
    }

    void RigidBody::RemoveConstraint(Constraint* constraint)
    {
        // ����Ʈ����Ʈ ����

        for (auto it = m_vecConstraints.begin(); it != m_vecConstraints.end(); it++)
        {
            const auto itConstraint = *it;
            if (constraint->GetObjectID() == itConstraint->GetObjectID())
            {
                it = m_vecConstraints.erase(it);
            }
        }

        Activate();
    }

    void RigidBody::SetShape(btCollisionShape* shape)
    {
        // �浹ü ��� ����

        m_CollisionShape = shape;

        if (m_CollisionShape)
        {
            BodyAddToWorld();
        }
        else
        {
            BodyRemoveFromWorld();
        }
    }

    void RigidBody::BodyAddToWorld()
    {
        if (m_Mass < 0.0f)
        {
            m_Mass = 0.0001f;
        }

        // �������� ���ο� �浹ü�� ������.
        btVector3 local_intertia = btVector3(0, 0, 0);
        if (m_CollisionShape && m_RigidBody)
        {
            local_intertia = m_RigidBody ? m_RigidBody->getLocalInertia() : local_intertia;
            m_CollisionShape->calculateLocalInertia(m_Mass, local_intertia);
        }

        BodyRelease();

        {
            // ���ο� ��ǻ��¸� ������.
            const auto motion_state = new MotionState(this);

            // ������ �����͸� �ִ´�.
            btRigidBody::btRigidBodyConstructionInfo constructionInfo(m_Mass, motion_state, m_CollisionShape, local_intertia);
            constructionInfo.m_mass = m_Mass;
            constructionInfo.m_friction = m_Friction;
            constructionInfo.m_rollingFriction = m_FrictionRolling;
            constructionInfo.m_restitution = m_Restitution;
            constructionInfo.m_collisionShape = m_CollisionShape;
            constructionInfo.m_localInertia = local_intertia;
            constructionInfo.m_motionState = motion_state;

            // ���ο� ������ �ٵ� ����
            m_RigidBody = new btRigidBody(constructionInfo);
            m_RigidBody->setUserPointer(this);
        }

        // ����Ʈ����Ʈ �缳��
        for (const auto& constraint : m_vecConstraints)
        {
            constraint->ApplyFrames();
        }

        FlagsUpdateKinematic();
        FlagsUpdateGravity();

        // Ʈ������
        SetPosition(GetTransform()->GetPosition());
        SetRotation(GetTransform()->GetRotation());

        // ����Ʈ����Ʈ
        SetPositionLock(m_PositionLock);
        SetRotationLock(m_RotationLock);

        // ��ġ, ȸ�� ��
        SetPositionLock(m_PositionLock);
        SetRotationLock(m_RotationLock);

        // ���忡 �߰��Ѵ�.
        m_Physics->AddBody(m_RigidBody);

        // ���� ��ġ�� �����Ѵٸ� Ȱ��ȭ
        if (m_Mass > 0.0f)
        {
            Activate();
        }
        // �ƴ϶�� 0���� ����
        else
        {
            SetLinearVelocity(Vector3::Zero);
            SetAngularVelocity(Vector3::Zero);
        }

        m_InWorld = true;
    }

    void RigidBody::BodyRelease()
    {
        if (!m_RigidBody)
            return;

        // ��� ����Ʈ����Ʈ�� �����Ѵ�.
        for (const auto& constraint : m_vecConstraints)
        {
            constraint->ReleaseConstraint();
        }

        // ���忡�� ����
        BodyRemoveFromWorld();

        m_RigidBody = nullptr;
    }

    void RigidBody::BodyRemoveFromWorld()
    {
        if (!m_RigidBody)
            return;

        if (m_InWorld)
        {
            m_Physics->RemoveBody(m_RigidBody);
            m_InWorld = false;
        }
    }

    void RigidBody::BodyAcquireShape()
    {
        if (const auto& collider = m_Entity->GetComponent<Collider>())
        {
            m_CollisionShape = collider->GetShape();
            m_CenterOfMass = collider->GetCenter();
        }
    }

    void RigidBody::FlagsUpdateKinematic() const
    {
        // Ű�׸�ƽ �÷��� ����

        int flags = m_RigidBody->getCollisionFlags();

        if (m_IsKinematic)
        {
            flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
        }
        else
        {
            flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
        }

        m_RigidBody->setCollisionFlags(flags);
        m_RigidBody->forceActivationState(m_IsKinematic ? DISABLE_DEACTIVATION : ISLAND_SLEEPING);
        m_RigidBody->setDeactivationTime(DEFAULT_DEACTIVATION_TIME);
    }

    void RigidBody::FlagsUpdateGravity() const
    {
        // �߷� ��� ���� ����

        int flags = m_RigidBody->getFlags();

        if (m_UseGravity)
        {
            flags &= ~BT_DISABLE_WORLD_GRAVITY;
        }
        else
        {
            flags |= BT_DISABLE_WORLD_GRAVITY;
        }

        m_RigidBody->setFlags(flags);

        if (m_UseGravity)
        {
            m_RigidBody->setGravity(ToBtVector3(m_Gravity));
        }
        else
        {
            m_RigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
        }
    }

    bool RigidBody::IsActivated() const
    {
        return m_RigidBody->isActive();
    }
}