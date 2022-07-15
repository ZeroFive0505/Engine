#include "Common.h"
#include "PhysicsDebugDraw.h"
#include "BulletPhysicsHelper.h"
#include "../Rendering/Renderer.h"

using namespace std;

namespace PlayGround
{
	PhysicsDebugDraw::PhysicsDebugDraw(Renderer* renderer)
	{
		m_Renderer = renderer;
		m_DebugMode = DBG_DrawWireframe | DBG_DrawContactPoints | DBG_DrawConstraints | DBG_DrawConstraintLimits;
	}

	void PhysicsDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor)
	{
		m_Renderer->DrawLine(ToVector3(from), ToVector3(to), ToVector4(fromColor), ToVector4(toColor));
	}

	void PhysicsDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
	{
		const btVector3& from = PointOnB;
		const btVector3 to = PointOnB + normalOnB * distance;
		drawLine(from, to, color);
	}

	void PhysicsDebugDraw::reportErrorWarning(const char* error_warning)
	{
		LOG_WARNING("%s", error_warning);
	}
}