#include "Common.h"
#include "Physics.h"
#include "PhysicsDebugDraw.h"
#include "BulletPhysicsHelper.h"
#include "../Profiling/Profiler.h"
#include "../Rendering/Renderer.h"
#include "../Core/Context.h"
#include "../Core/Settings.h"
#include "../Core/Engine.h"
#include "../World/World.h"

#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	static const bool m_SoftBodySupport = true;

	// Bullet 초기화
	Physics::Physics(Context* context) : SubModule(context)
	{
		m_Broadphase = new btDbvtBroadphase();
		m_ConstraintSolver = new btSequentialImpulseConstraintSolver();

		// 소프트 바디 지원시
		if (m_SoftBodySupport)
		{
			m_CollisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();
			m_CollisionDispatcher = new btCollisionDispatcher(m_CollisionConfiguration);
			m_World = new btSoftRigidDynamicsWorld(m_CollisionDispatcher, m_Broadphase, m_ConstraintSolver, m_CollisionConfiguration);

			m_WorldInfo = new btSoftBodyWorldInfo();
			m_WorldInfo->m_sparsesdf.Initialize();
			m_World->getDispatchInfo().m_enableSPU = true;
			m_WorldInfo->m_dispatcher = m_CollisionDispatcher;
			m_WorldInfo->m_broadphase = m_Broadphase;
			m_WorldInfo->air_density = (btScalar)1.2;
			m_WorldInfo->water_density = 0;
			m_WorldInfo->water_offset = 0;
			m_WorldInfo->water_normal = btVector3(0, 0, 0);
			m_WorldInfo->m_gravity = ToBtVector3(m_Gravity);
		}
		// 아닐시
		else
		{
			m_CollisionConfiguration = new btDefaultCollisionConfiguration();
			m_CollisionDispatcher = new btCollisionDispatcher(m_CollisionConfiguration);
			m_World = new btDiscreteDynamicsWorld(m_CollisionDispatcher, m_Broadphase, m_ConstraintSolver, m_CollisionConfiguration);
		}

		m_World->setGravity(ToBtVector3(m_Gravity));
		m_World->getDispatchInfo().m_useContinuous = true;
		m_World->getSolverInfo().m_splitImpulse = false;
		m_World->getSolverInfo().m_numIterations = m_MaxSolveIterations;
	}

	Physics::~Physics()
	{
		SAFE_DELETE(m_World);
		SAFE_DELETE(m_ConstraintSolver);
		SAFE_DELETE(m_CollisionDispatcher);
		SAFE_DELETE(m_CollisionConfiguration);
		SAFE_DELETE(m_Broadphase);
		SAFE_DELETE(m_WorldInfo);
		SAFE_DELETE(m_DebugDraw);
	}

	void Physics::OnInit()
	{
		m_Renderer = m_Context->GetSubModule<Renderer>();
		m_Profiler = m_Context->GetSubModule<Profiler>();

		const string major = to_string(btGetVersion() / 100);
		const string minor = to_string(btGetVersion()).erase(0, 1);

		m_Context->GetSubModule<Settings>()->RegisterThirdParty("Bullet", major + "." + minor, "https://github.com/bulletphysics/bullet3");


		m_DebugDraw = new PhysicsDebugDraw(m_Renderer);

		if (m_World)
			m_World->setDebugDrawer(m_DebugDraw);
	}

	void Physics::Update(double delta_time)
	{
		// 만약 아직 월드가 존재하지 않는다면 반환
		if (!m_World)
			return;

		// 디버깅용 렌더링
		if (m_Renderer->GetOptions() & Renderer::Option::Debug_Physics)
			m_World->debugDrawWorld();


		// 특정 상황이 아니라면 그냥 반환한다.
		if (!m_Context->m_Engine->IsEngineModeSet(PhysicsMode) ||
			!m_Context->m_Engine->IsEngineModeSet(GameMode))
			return;

		// 한 프레임씩만 업데이트
		if (m_Context->m_Engine->IsEngineModeSet(PauseMode) && !m_Context->GetSubModule<World>()->GetUpdateOnce())
			return;

		// 프로파일링 시작
		SCOPED_TIME_BLOCK(m_Profiler);

		// 최대치를 구한다.
		float internal_time_step = 1.0f / m_InternalFPS;
		int max_substeps = static_cast<int>(delta_time * m_InternalFPS) + 1;

		if (m_MaxSubSteps < 0)
		{
			internal_time_step = static_cast<float>(delta_time);
			max_substeps = 1;
		}
		else if (m_MaxSubSteps > 0)
			max_substeps = Util::Min(max_substeps, m_MaxSubSteps);

		// 물리 시뮬레이션 시작
		m_IsSimulating = true;
		m_World->stepSimulation(static_cast<float>(delta_time), max_substeps, internal_time_step);
		m_IsSimulating = false;
	}

	void Physics::AddBody(btRigidBody* body) const
	{
		if (!m_World)
			return;

		m_World->addRigidBody(body);
	}

	void Physics::RemoveBody(btRigidBody*& body) const
	{
		if (!m_World)
			return;

		m_World->removeRigidBody(body);
		delete body->getMotionState();
		SAFE_DELETE(body);
	}

	void Physics::AddConstraint(btTypedConstraint* constraint, bool collision_with_linked_body /*= true*/) const
	{
		if (!m_World)
			return;

		m_World->addConstraint(constraint, !collision_with_linked_body);
	}

	void Physics::RemoveConstraint(btTypedConstraint*& constraint) const
	{
		if (!m_World)
			return;

		m_World->removeConstraint(constraint);
		SAFE_DELETE(constraint);
	}

	void Physics::AddBody(btSoftBody* body) const
	{
		if (!m_World)
			return;

		btSoftRigidDynamicsWorld* world = static_cast<btSoftRigidDynamicsWorld*>(m_World);

		if (world)
			world->addSoftBody(body);
	}

	void Physics::RemoveBody(btSoftBody*& body) const
	{
		btSoftRigidDynamicsWorld* world = static_cast<btSoftRigidDynamicsWorld*>(m_World);

		if (world)
		{
			world->removeSoftBody(body);
			SAFE_DELETE(body);
		}
	}

	Vector3 Physics::GetGravity() const
	{
		auto gravity = m_World->getGravity();
		if (!gravity)
		{
			LOG_ERROR("Unable to get gravity, ensure physics are properly initialized.");
			return Vector3::Zero;
		}
		return gravity ? ToVector3(gravity) : Vector3::Zero;
	}
}