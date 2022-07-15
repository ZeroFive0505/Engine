#pragma once

#include <vector>
#include "../Core/EventSystem.h"
#include "Components/IComponent.h"

namespace PlayGround
{
	class Context;
	class Transform;
	class Renderable;
	
	// ���ӳ� ��ġ�Ǵ� ������Ʈ���� ��Ÿ���� Ŭ����
	class Entity : public EngineObject, public std::enable_shared_from_this<Entity>
	{
	public:
		Entity(Context* context, uint64_t transform_id = 0);
		~Entity();

		// ����
		void Clone();

		// ���۽�
		void OnStart();

		// ������
		void OnStop();

		// ������Ʈ ����
		void PrevUpdate();

		// ������Ʈ
		void Update(double delta_time);

		// ����
		void Serialize(FileStream* stream);
		// �ҷ�����
		void Deserialize(FileStream* stream, Transform* parent);

		inline const std::string& GetObjectName() const { return m_ObjectName; }

		inline void SetName(const std::string& name) { m_ObjectName = name; }

		inline bool IsActive() const { return m_IsActive; }

		inline void SetActive(const bool active) { m_IsActive = active; }

		inline bool IsVisibleInHierarchy() const { return m_Hierarchy_visibility; }

		inline void SetHierarchyVisibility(const bool hierarchy_visibility) { m_Hierarchy_visibility = hierarchy_visibility; }

		// TŸ���� ������Ʈ�� �߰��Ѵ�.
		template <typename T>
		T* AddComponent(uint64_t id = 0)
		{
			const EComponentType type = IComponent::TypeToEnum<T>();
			
			// ���� �̹� ������ �ִ� ������Ʈ��� ��ȯ�Ѵ�.
			if (HasComponent(type))
				return GetComponent<T>();

			// ���ο� ������Ʈ ����
			std::shared_ptr<T> component = std::make_shared<T>(m_Context, this, id);

			// �߰��Ѵ�.
			m_vecComponents.emplace_back(std::static_pointer_cast<IComponent>(component));
			// ����ũ ��
			m_ComponentMask |= GetComponentMask(type);

			// Ʈ�������� ��� ���� �����صд�.
			if constexpr (std::is_same<T, Transform>::value)
			{
				m_Transform = static_cast<Transform*>(component.get());
			}

			// �������� ������Ʈ�� ��������
			if constexpr (std::is_same<T, Renderable>::value)
			{
				m_Renderable = static_cast<Renderable*>(component.get());
			}

			// ������Ʈ Ÿ�� ����
			component->SetComponentType(type);
			// �ʱ�ȭ
			component->OnInit();

			// ���� �̺�Ʈ �߻�
			FIRE_EVENT(EventType::WorldResolve);

			return component.get();
		}

		IComponent* AddComponent(EComponentType type, uint64_t id = 0);

		// TŸ���� ������Ʈ�� �����´�.
		template <typename T>
		T* GetComponent()
		{
			// ���� ����ü�� ��ȯ
			const EComponentType type = IComponent::TypeToEnum<T>();

			// ���� ������ ���� �ʴٸ� ��ȯ
			if (!HasComponent(type))
				return nullptr;

			// Ÿ�� �� �� �˸��� ������Ʈ ��ȯ
			for (std::shared_ptr<IComponent>& component : m_vecComponents)
			{
				if (component->GetComponentType() == type)
					return static_cast<T*>(component.get());
			}

			return nullptr;
		}

		// ��� ������Ʈ���� ��ȯ�Ѵ�.
		template <typename T>
		std::vector<T*> GetComponents()
		{
			std::vector<T*> components;
			const EComponentType type = IComponent::TypeToEnum<T>();

			if (!HasComponent(type))
				return components;

			for (std::shared_ptr<IComponent>& component : m_vecComponents)
			{
				if (component->GetComponentType() != type)
					continue;

				components.emplace_back(static_cast<T*>(component.get()));
			}

			return components;
		}

		// �ص� �������� ������Ʈ ���� ���� Ȯ��
		inline constexpr bool HasComponent(const EComponentType type) { return m_ComponentMask & GetComponentMask(type); }

		// TŸ���� ������Ʈ�� �����ϰ� �ִ��� Ȯ��
		template <typename T>
		bool HasComponent() { return HasComponent(IComponent::TypeToEnum<T>()); }

		// T Ÿ���� ������Ʈ ����
		template <typename T>
		void RemoveComponent()
		{
			// ����ü�� ��ȯ�Ѵ�.
			const EComponentType type = IComponent::TypeToEnum<T>();

			for (auto it = m_vecComponents.begin(); it != m_vecComponents.end(); it++)
			{
				auto component = *it;
				// ���� ������Ʈ�� �߰��ߴٸ�
				if (component->GetComponentType() == type)
				{
					// ������ ȣ��Ǵ� �Լ� ȣ��
					component->OnRemove();
					// ����
					it = m_vecComponents.erase(it);
					// ����ũ ���������� �����Ѵ�.
					m_ComponentMask &= ~GetComponentMask(type);
					break;
				}
			}

			// �̺�Ʈ �߻�
			FIRE_EVENT(EventType::WorldResolve);
		}

		void RemoveComponentByID(uint64_t id);
		inline const auto& GetAllComponents() const { return m_vecComponents; }

		inline void MarkForDestruction() { m_Destruction_pending = true; }

		inline bool IsPendingDestruction() const { return m_Destruction_pending; }

		inline Transform* GetTransform() const { return m_Transform; }

		inline Renderable* GetRenderable() const { return m_Renderable; }

		inline std::shared_ptr<Entity> GetSharedPtr() { return shared_from_this(); }

	private:
		inline constexpr uint32_t GetComponentMask(EComponentType type) { return static_cast<uint32_t>(1) << static_cast<uint32_t>(type); }

		std::string m_ObjectName = "Entity";
		bool m_IsActive = true;
		bool m_Hierarchy_visibility = true;
		Transform* m_Transform = nullptr;
		Renderable* m_Renderable = nullptr;
		bool m_Destruction_pending = false;

		std::vector<std::shared_ptr<IComponent>> m_vecComponents;
		uint32_t m_ComponentMask = 0;
	};
}