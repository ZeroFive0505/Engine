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
		// ��
		auto scene = m_Context->GetSubModule<World>();
		vector<Entity*> clones;

		// ���� �����Լ�
		auto clone_entity = [&scene, &clones](Entity* entity)
		{
			// ���� ������ �� ��ƼƼ �ϳ��� �����Ѵ�.
			auto clone = scene->EntityCreate().get();
			// ���̵�� �̸� �� �Ӽ����� �����Ѵ�.
			clone->SetObjectID(GenerateObjectID());
			clone->SetName(entity->GetObjectName());
			clone->SetActive(entity->IsActive());
			clone->SetHierarchyVisibility(entity->IsVisibleInHierarchy());

			// ��� ������Ʈ�� ��ȸ�ϸ鼭
			for (const auto& component : entity->GetAllComponents())
			{
				// ������Ʈ�� �����Ѵ�.
				const auto& original_comp = component;
				auto clone_comp = clone->AddComponent(component->GetComponentType());
				clone_comp->SetAttributes(original_comp->GetAttributes());
			}

			// �׸��� ����� ��ƼƼ�� ��´�.
			clones.emplace_back(clone);

			return clone;
		};

		// ��ƼƼ �ڽ��� �����ϴ� ����
		function<Entity* (Entity*)> clone_entity_and_descendants = [&clone_entity_and_descendants, &clone_entity](Entity* original)
		{
			// �ڱ��ڽ��� �����Ѵ�.
			const auto clone_self = clone_entity(original);

			// ��� �ڽ��� ��ȸ�Ѵ�.
			for (const auto& child_transform : original->GetTransform()->GetChildren())
			{
				// �׸��� �����Ѵ�.
				const auto clone_child = clone_entity_and_descendants(child_transform->GetEntity());
				// ����� �ڽ��� �θ� �����Ѵ�.
				clone_child->GetTransform()->SetParent(clone_self->GetTransform());
			}

			// ��ȯ
			return clone_self;
		};

		// �ڱ� �ڽ��� �ڽ��� �����Ѵ�.
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
		// ��ƼƼ�� ���� ����

		{
			stream->Write(m_IsActive);
			stream->Write(m_Hierarchy_visibility);
			stream->Write(GetObjectID());
			stream->Write(m_ObjectName);
		}

		{
			// ���� ������Ʈ ������ �����Ѵ�.
			stream->Write(static_cast<uint32_t>(m_vecComponents.size()));

			// ��� ������Ʈ�� ��ȸ�ϸ鼭
			for (auto& component : m_vecComponents)
			{
				// Ÿ�԰� ���̵� �����Ѵ�.
				stream->Write(static_cast<uint32_t>(component->GetComponentType()));
				stream->Write(component->GetObjectID());
			}

			// �׸��� ������Ʈ�� �����Ѵ�.
			for (auto& component : m_vecComponents)
			{
				component->Serialize(stream);
			}
		}

		// ���� ��ƼƼ�� �ڽ��� �����Ѵ�.

		{
			// ��� �ڽ��� �����´�.
			vector<Transform*>& children = GetTransform()->GetChildren();

			// �ڽ��� ���� ���� �����Ѵ�.
			stream->Write(static_cast<uint32_t>(children.size()));

			// ��� �ڽ��� ��ȸ�ϸ鼭 ID�� �����Ѵ�.
			for (Transform* child : children)
			{
				stream->Write(child->GetObjectID());
			}

			// �ڽ��� ��ƼƼ�� �����Ѵ�.
			for (Transform* child : children)
			{
				if (child->GetEntity())
					child->GetEntity()->Serialize(stream);
			}
		}
	}

	void Entity::Deserialize(FileStream* stream, Transform* parent)
	{
		// ��ƼƼ �ҷ�����

		{
			stream->Read(&m_IsActive);
			stream->Read(&m_Hierarchy_visibility);
			stream->Read(&m_ObjectID);
			stream->Read(&m_ObjectName);
		}

		{
			// ���� �ҷ��� ��ƼƼ�� ������ �ִ� ������Ʈ�� ������ �ҷ��´�.
			const uint32_t component_count = stream->ReadAs<uint32_t>();

			// ������ŭ �ݺ��Ѵ�.
			for (uint32_t i = 0; i < component_count; i++)
			{
				// ������Ʈ Ÿ�԰� ���̵� �ҷ��´�.
				uint32_t component_type = static_cast<uint32_t>(EComponentType::Unknown);
				uint64_t component_id = 0;

				stream->Read(&component_type);
				stream->Read(&component_id);

				// �ҷ��� ���̵�� Ÿ������ ������Ʈ�� �߰��Ѵ�.
				AddComponent(static_cast<EComponentType>(component_type), component_id);
			}

			// �׸��� ������Ʈ�� ������ �ҷ��´�.
			for (auto& component : m_vecComponents)
			{
				component->Deserialize(stream);
			}

			// Ʈ������ ������Ʈ�� ���� �θ� �������ش�.
			if (m_Transform)
				m_Transform->SetParent(parent);
		}

		{
			// �ڽ� ��ƼƼ�� ������ �����´�.
			const uint32_t children_count = stream->ReadAs<uint32_t>();

			// ���� �����´�.
			World* world = m_Context->GetSubModule<World>();
			vector<weak_ptr<Entity>> children;

			// ������ŭ ���ο� ��ƼƼ�� �����ϰ� �߰��Ѵ�.
			for (uint32_t i = 0; i < children_count; i++)
			{
				shared_ptr<Entity> child = world->EntityCreate();

				child->SetObjectID(stream->ReadAs<uint64_t>());

				children.emplace_back(child);
			}

			// �Ӽ��� �ҷ��´�.
			for (const auto& child : children)
			{
				child.lock()->Deserialize(stream, GetTransform());
			}

			// ���� Ʈ������ ������Ʈ�� ������� ��� �ڽ��� ��ã�ƿ´�.
			if (m_Transform)
				m_Transform->AcquireChildren();
		}

		// �̺�Ʈ �߻�
		FIRE_EVENT(EventType::WorldResolve);
	}

	// Ÿ���� �������� ������Ʈ�� �߰��Ѵ�.
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

	// ���̵� �������� ������Ʈ�� �����Ѵ�.
	void Entity::RemoveComponentByID(const uint64_t id)
	{
		EComponentType component_type = EComponentType::Unknown;

		for (auto it = m_vecComponents.begin(); it != m_vecComponents.end(); it++)
		{
			auto component = *it;

			// �ش� ���̵� ã���� ��� �����Ѵ�.
			if (id == component->GetObjectID())
			{
				component_type = component->GetComponentType();
				component->OnRemove();
				it = m_vecComponents.erase(it);
				break;
			}
		}

		// ���������� ����ũ ���������� ����
		m_ComponentMask &= ~GetComponentMask(component_type);

		// �̺�Ʈ �߻�
		FIRE_EVENT(EventType::WorldResolve);

	}
}