#pragma once

#include <memory>
#include "../EngineDefinition.h"

namespace PlayGround
{
	class Entity;

	namespace Math
	{
		class RayHit
		{
		public:
			RayHit(const std::shared_ptr<Entity>& entity, const Vector3& position, float distance, bool is_inside)
			{
				m_Entity = entity;
				m_Position = position;
				m_Distance = distance;
				m_Inside = is_inside;
			}

			std::shared_ptr<Entity> m_Entity;
			Vector3 m_Position;
			float m_Distance;
			bool m_Inside;
		};
	}
}