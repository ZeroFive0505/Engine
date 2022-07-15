#pragma once

#include "../Core/SubModule.h"
#include "../Math/Vector3.h"

class btBroadphaseInterface;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDefaultCollisionConfiguration;
class btCollisionObject;
class btDiscreteDynamicsWorld;
class btRigidBody;
class btSoftBody;
class btTypedConstraint;
struct btSoftBodyWorldInfo;

namespace PlayGround
{
	class Renderer;
	class PhysicsDebugDraw;
	class Profiler;
	namespace Math 
	{ 
		class Vector3;
	}


	// ���� ������ ���� ���
	class Physics : public SubModule
	{
	public:
		Physics(Context* context);
		~Physics();

		void OnInit() override;
		void Update(double delta_time) override;

		// ��ü�� ���忡 �߰��Ѵ�.
		void AddBody(btRigidBody* body) const;
		void RemoveBody(btRigidBody*& body) const;

		// �������� ���忡 �߰��Ѵ�.
		void AddBody(btSoftBody* body) const;
		void RemoveBody(btSoftBody*& body) const;

		// ����Ʈ����Ʈ �߰�
		void AddConstraint(btTypedConstraint* constraint, bool collision_with_linked_body = true) const;
		void RemoveConstraint(btTypedConstraint*& constraint) const;

		// �߷� ��ġ ��ȯ
		Math::Vector3 GetGravity() const;

		// ���� ������ ����Ǵ� ���带 ��ȯ�Ѵ�.
		inline btSoftBodyWorldInfo& GetSoftWorldInfo() const { return *m_WorldInfo; }

		// ������ ������
		inline PhysicsDebugDraw* GetPhysicsDebugDraw() const { return m_DebugDraw; }

		// ���� �ùķ��̼� ����
		inline bool IsSimulating() const { return m_IsSimulating; }

	private:
		btBroadphaseInterface* m_Broadphase = nullptr;
		btCollisionDispatcher* m_CollisionDispatcher = nullptr;
		btSequentialImpulseConstraintSolver* m_ConstraintSolver = nullptr;
		btDefaultCollisionConfiguration* m_CollisionConfiguration = nullptr;
		btDiscreteDynamicsWorld* m_World = nullptr;
		btSoftBodyWorldInfo* m_WorldInfo = nullptr;
		PhysicsDebugDraw* m_DebugDraw = nullptr;

		Renderer* m_Renderer = nullptr;
		Profiler* m_Profiler = nullptr;

		int m_MaxSubSteps = 1;
		int m_MaxSolveIterations = 256;
		float m_InternalFPS = 60.0f;
		Math::Vector3 m_Gravity = Math::Vector3(0.0f, -9.81f, 0.0f);
		bool m_IsSimulating = false;
	};
}
