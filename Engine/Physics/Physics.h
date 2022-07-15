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


	// 물리 연산을 위한 모듈
	class Physics : public SubModule
	{
	public:
		Physics(Context* context);
		~Physics();

		void OnInit() override;
		void Update(double delta_time) override;

		// 강체를 월드에 추가한다.
		void AddBody(btRigidBody* body) const;
		void RemoveBody(btRigidBody*& body) const;

		// 연조직을 월드에 추가한다.
		void AddBody(btSoftBody* body) const;
		void RemoveBody(btSoftBody*& body) const;

		// 컨스트레인트 추가
		void AddConstraint(btTypedConstraint* constraint, bool collision_with_linked_body = true) const;
		void RemoveConstraint(btTypedConstraint*& constraint) const;

		// 중력 수치 반환
		Math::Vector3 GetGravity() const;

		// 물리 연산이 수행되는 월드를 반환한다.
		inline btSoftBodyWorldInfo& GetSoftWorldInfo() const { return *m_WorldInfo; }

		// 디버깅용 렌더러
		inline PhysicsDebugDraw* GetPhysicsDebugDraw() const { return m_DebugDraw; }

		// 현재 시뮬레이션 여부
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
