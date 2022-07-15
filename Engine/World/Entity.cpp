#include "Common.h"
#include "Entity.h"
#include "World.h"
#include "Components/Camera.h"
#include "Components/Collider.h"
#include "Components/Transform.h"
#include "Components/Constraint.h"
#include "Components/Light.h"
#include "Components/Renderable.h"
#include "Components/RigidBody.h"
#include "Components/Environment.h"
#include "Components/AudioSource.h" 
#include "Components/AudioListener.h"
#include "Components/Terrain.h"
#include "Components/ReflectionProbe.h"
#include "../IO/FileStream.h"

#include "../Core/Context.h"

using namespace std;

namespace PlayGround
{
	Entity::Entity(Context* context, uint64_t transform_id /*= 0*/)
	{
		m_Context = context;
		m_ObjectName = "Entity";
		m_IsActive = true;
		m_Hierarchy_visibility = true;
		AddComponent<Transform>(transform_id);
	}

	Entity::~Entity()
	{
		for (auto it = m_vecComponents.begin(); it != m_vecComponents.end();)
		{
			(*it)->OnRemove();
			(*it).reset();
			it = m_vecComponents.erase(it);
		}
	}

	void Entity::Clone()
	{
		// 씬
		auto scene = m_Context->GetSubModule<World>();
		vector<Entity*> clones;

		// 복사 람다함수
		auto clone_entity = [&scene, &clones](Entity* entity)
		{
			// 현재 씬에서 빈 엔티티 하나를 생성한다.
			auto clone = scene->EntityCreate().get();
			// 아이디와 이름 등 속성등을 설정한다.
			clone->SetObjectID(GenerateObjectID());
			clone->SetName(entity->GetObjectName());
			clone->SetActive(entity->IsActive());
			clone->SetHierarchyVisibility(entity->IsVisibleInHierarchy());

			// 모든 컴포넌트를 순회하면서
			for (const auto& component : entity->GetAllComponents())
			{
				// 컴포넌트도 복사한다.
				const auto& original_comp = component;
				auto clone_comp = clone->AddComponent(component->GetComponentType());
				clone_comp->SetAttributes(original_comp->GetAttributes());
			}

			// 그리고 복사된 엔티티를 담는다.
			clones.emplace_back(clone);

			return clone;
		};

		// 엔티티 자식을 복사하는 람다
		function<Entity* (Entity*)> clone_entity_and_descendants = [&clone_entity_and_descendants, &clone_entity](Entity* original)
		{
			// 자기자신을 복사한다.
			const auto clone_self = clone_entity(original);

			// 모든 자식을 순회한다.
			for (const auto& child_transform : original->GetTransform()->GetChildren())
			{
				// 그리고 복사한다.
				const auto clone_child = clone_entity_and_descendants(child_transform->GetEntity());
				// 복사된 자식의 부모를 설정한다.
				clone_child->GetTransform()->SetParent(clone_self->GetTransform());
			}

			// 반환
			return clone_self;
		};

		// 자기 자신의 자식을 복사한다.
		clone_entity_and_descendants(this);
	}

	void Entity::OnStart()
	{
		for (const auto& component : m_vecComponents)
		{
			component->OnStart();
		}
	}

	void Entity::OnStop()
	{
		for (const auto& component : m_vecComponents)
		{
			component->OnStop();
		}
	}

	void Entity::PrevUpdate()
	{

	}

	void Entity::Update(double delta_time)
	{
		if (!m_IsActive)
			return;

		for (auto& component : m_vecComponents)
		{
			component->Update(delta_time);
		}
	}

	void Entity::Serialize(FileStream* stream)
	{
		// 엔티티의 설정 저장

		{
			stream->Write(m_IsActive);
			stream->Write(m_Hierarchy_visibility);
			stream->Write(GetObjectID());
			stream->Write(m_ObjectName);
		}

		{
			// 먼저 컴포넌트 갯수를 저장한다.
			stream->Write(static_cast<uint32_t>(m_vecComponents.size()));

			// 모든 컴포넌트를 순회하면서
			for (auto& component : m_vecComponents)
			{
				// 타입과 아이디를 저장한다.
				stream->Write(static_cast<uint32_t>(component->GetComponentType()));
				stream->Write(component->GetObjectID());
			}

			// 그리고 컴포넌트를 저장한다.
			for (auto& component : m_vecComponents)
			{
				component->Serialize(stream);
			}
		}

		// 현재 엔티티의 자식을 저장한다.

		{
			// 모든 자식을 가져온다.
			vector<Transform*>& children = GetTransform()->GetChildren();

			// 자식의 수를 먼저 저장한다.
			stream->Write(static_cast<uint32_t>(children.size()));

			// 모든 자식을 순회하면서 ID를 저장한다.
			for (Transform* child : children)
			{
				stream->Write(child->GetObjectID());
			}

			// 자식의 엔티티를 저장한다.
			for (Transform* child : children)
			{
				if (child->GetEntity())
					child->GetEntity()->Serialize(stream);
			}
		}
	}

	void Entity::Deserialize(FileStream* stream, Transform* parent)
	{
		// 엔티티 불러오기

		{
			stream->Read(&m_IsActive);
			stream->Read(&m_Hierarchy_visibility);
			stream->Read(&m_ObjectID);
			stream->Read(&m_ObjectName);
		}

		{
			// 먼저 불러올 엔티티가 가지고 있는 컴포넌트의 갯수를 불러온다.
			const uint32_t component_count = stream->ReadAs<uint32_t>();

			// 갯수만큼 반복한다.
			for (uint32_t i = 0; i < component_count; i++)
			{
				// 컴포넌트 타입과 아이디를 불러온다.
				uint32_t component_type = static_cast<uint32_t>(EComponentType::Unknown);
				uint64_t component_id = 0;

				stream->Read(&component_type);
				stream->Read(&component_id);

				// 불러온 아이디와 타입으로 컴포넌트를 추가한다.
				AddComponent(static_cast<EComponentType>(component_type), component_id);
			}

			// 그리고 컴포넌트의 설정을 불러온다.
			for (auto& component : m_vecComponents)
			{
				component->Deserialize(stream);
			}

			// 트랜스폼 컴포넌트의 경우는 부모를 설정해준다.
			if (m_Transform)
				m_Transform->SetParent(parent);
		}

		{
			// 자식 엔티티의 갯수를 가져온다.
			const uint32_t children_count = stream->ReadAs<uint32_t>();

			// 씬을 가져온다.
			World* world = m_Context->GetSubModule<World>();
			vector<weak_ptr<Entity>> children;

			// 갯수만큼 새로운 엔티티를 생성하고 추가한다.
			for (uint32_t i = 0; i < children_count; i++)
			{
				shared_ptr<Entity> child = world->EntityCreate();

				child->SetObjectID(stream->ReadAs<uint64_t>());

				children.emplace_back(child);
			}

			// 속성을 불러온다.
			for (const auto& child : children)
			{
				child.lock()->Deserialize(stream, GetTransform());
			}

			// 만약 트랜스폼 컴포넌트가 있을경우 모든 자식을 되찾아온다.
			if (m_Transform)
				m_Transform->AcquireChildren();
		}

		// 이벤트 발생
		FIRE_EVENT(EventType::WorldResolve);
	}

	// 타입을 기준으로 컴포넌트를 추가한다.
	IComponent* Entity::AddComponent(const EComponentType type, uint64_t id /*= 0*/)
	{
		IComponent* component = nullptr;

		switch (type)
		{
		case EComponentType::Camera:          component = static_cast<IComponent*>(AddComponent<Camera>(id));          break;
		case EComponentType::Collider:        component = static_cast<IComponent*>(AddComponent<Collider>(id));        break;
		case EComponentType::Light:           component = static_cast<IComponent*>(AddComponent<Light>(id));           break;
		case EComponentType::Renderable:      component = static_cast<IComponent*>(AddComponent<Renderable>(id));      break;
		case EComponentType::RigidBody:       component = static_cast<IComponent*>(AddComponent<RigidBody>(id));       break;
		case EComponentType::Environment:     component = static_cast<IComponent*>(AddComponent<Environment>(id));     break;
		case EComponentType::Transform:       component = static_cast<IComponent*>(AddComponent<Transform>(id));       break;
		case EComponentType::Unknown:         component = nullptr;                                                     break;
		default:                              component = nullptr;                                                     break;
		}

		ASSERT(component != nullptr);

		return component;

	}

	// 아이디를 기준으로 컴포넌트를 삭제한다.
	void Entity::RemoveComponentByID(const uint64_t id)
	{
		EComponentType component_type = EComponentType::Unknown;

		for (auto it = m_vecComponents.begin(); it != m_vecComponents.end(); it++)
		{
			auto component = *it;

			// 해당 아이디를 찾았을 경우 삭제한다.
			if (id == component->GetObjectID())
			{
				component_type = component->GetComponentType();
				component->OnRemove();
				it = m_vecComponents.erase(it);
				break;
			}
		}

		// 삭제됬으니 마스크 변수에서도 삭제
		m_ComponentMask &= ~GetComponentMask(component_type);

		// 이벤트 발생
		FIRE_EVENT(EventType::WorldResolve);

	}
}