#pragma once

#include <LinearMath/btIDebugDraw.h>

namespace PlayGround
{
	class Renderer;

	// 물리 연산시 디버깅을 위한 클래스 
	// Bullet에서 제공하는 클래스를 상속받는다.
	class PhysicsDebugDraw : public btIDebugDraw
	{
	public:
		PhysicsDebugDraw(Renderer* renderer);
		~PhysicsDebugDraw() = default;

		void drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor) override;
		void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override { drawLine(from, to, color, color); }
		void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
		void reportErrorWarning(const char* warningString) override;
		void draw3dText(const btVector3& location, const char* textString) override {}
		void setDebugMode(const int debugMode) override { m_DebugMode = debugMode; }
		int getDebugMode() const override { return m_DebugMode; }

	private:
		Renderer* m_Renderer;
		int m_DebugMode;
	};
}

