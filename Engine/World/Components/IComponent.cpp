#include "Common.h"
#include "IComponent.h"
#include "Light.h"
#include "Environment.h"
#include "RigidBody.h"
#include "Collider.h"
#include "Constraint.h"
#include "Camera.h"
#include "AudioSource.h"
#include "AudioListener.h"
#include "Renderable.h"
#include "Transform.h"
#include "Terrain.h"
#include "ReflectionProbe.h"
#include "../Entity.h"

using namespace std;

namespace PlayGround
{
	IComponent::IComponent(Context* context, Entity* entity, uint64_t id, Transform* transform)
	{
		m_Context = context;
		m_Entity = entity;
		m_Transform = transform ? transform : entity->GetTransform();
		m_Enabled = true;
	}

	string IComponent::GetEntityName() const
	{
		if (!m_Entity)
			return "";
		
		return m_Entity->GetObjectName();
	}

	template <typename T>
	inline constexpr EComponentType IComponent::TypeToEnum() { return EComponentType::Unknown; }

	// T타입을 상속받는지를 체크한다.
	template <typename T>
	inline constexpr void Validate_component_type() { static_assert(std::is_base_of<IComponent, T>::value, "Provided type does not implement IComponent"); }

	// 탬플릿 특수화
	#define REGISTER_COMPONENT(T, enumT) template<> EComponentType IComponent::TypeToEnum<T>() { Validate_component_type<T>(); return enumT;}

	REGISTER_COMPONENT(Camera, EComponentType::Camera)
	REGISTER_COMPONENT(Collider, EComponentType::Collider)
	REGISTER_COMPONENT(ReflectionProbe, EComponentType::ReflectionProbe)
	REGISTER_COMPONENT(Light, EComponentType::Light)
	REGISTER_COMPONENT(Renderable, EComponentType::Renderable)
	REGISTER_COMPONENT(RigidBody, EComponentType::RigidBody)
	REGISTER_COMPONENT(Environment, EComponentType::Environment)
	REGISTER_COMPONENT(Transform, EComponentType::Transform)
}