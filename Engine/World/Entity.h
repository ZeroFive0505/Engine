#pragma once

#include <vector>
#include "../Core/EventSystem.h"
#include "Components/IComponent.h"

namespace PlayGround
{
	class Context;
	class Transform;
	class Renderable;
	
	// 게임내 배치되는 오브젝트들을 나타내는 클래스
	class Entity : public EngineObject, public std::enable_shared_from_this<Entity>
	{
	public:
		Entity(Context* context, uint64_t transform_id = 0);
		~Entity();

		// 복사
		void Clone();

		// 시작시
		void OnStart();

		// 정지시
		void OnStop();

		// 업데이트 이전
		void PrevUpdate();

		// 업데이트
		void Update(double delta_time);

		// 저장
		void Serialize(FileStream* stream);
		// 불러오기
		void Deserialize(FileStream* stream, Transform* parent);

		inline const std::string& GetObjectName() const { return m_ObjectName; }

		inline void SetName(const std::string& name) { m_ObjectName = name; }

		inline bool IsActive() const { return m_IsActive; }

		inline void SetActive(const bool active) { m_IsActive = active; }

		inline bool IsVisibleInHierarchy() const { return m_Hierarchy_visibility; }

		inline void SetHierarchyVisibility(const bool hierarchy_visibility) { m_Hierarchy_visibility = hierarchy_visibility; }

		// T타입의 컴포넌트를 추가한다.
		template <typename T>
		T* AddComponent(uint64_t id = 0)
		{
			const EComponentType type = IComponent::TypeToEnum<T>();
			
			// 만약 이미 가지고 있는 컴포넌트라면 반환한다.
			if (HasComponent(type))
				return GetComponent<T>();

			// 새로운 컴포넌트 생성
			std::shared_ptr<T> component = std::make_shared<T>(m_Context, this, id);

			// 추가한다.
			m_vecComponents.emplace_back(std::static_pointer_cast<IComponent>(component));
			// 마스크 온
			m_ComponentMask |= GetComponentMask(type);

			// 트랜스폼의 경우 따로 저장해둔다.
			if constexpr (std::is_same<T, Transform>::value)
			{
				m_Transform = static_cast<Transform*>(component.get());
			}

			// 렌더러블 컴포넌트도 마찬가지
			if constexpr (std::is_same<T, Renderable>::value)
			{
				m_Renderable = static_cast<Renderable*>(component.get());
			}

			// 컴포넌트 타입 설정
			component->SetComponentType(type);
			// 초기화
			component->OnInit();

			// 월드 이벤트 발생
			FIRE_EVENT(EventType::WorldResolve);

			return component.get();
		}

		IComponent* AddComponent(EComponentType type, uint64_t id = 0);

		// T타입의 컴포넌트를 가져온다.
		template <typename T>
		T* GetComponent()
		{
			// 먼저 열거체로 변환
			const EComponentType type = IComponent::TypeToEnum<T>();

			// 만약 가지고 있지 않다면 반환
			if (!HasComponent(type))
				return nullptr;

			// 타입 비교 후 알맞은 컴포넌트 반환
			for (std::shared_ptr<IComponent>& component : m_vecComponents)
			{
				if (component->GetComponentType() == type)
					return static_cast<T*>(component.get());
			}

			return nullptr;
		}

		// 모든 컴포넌트들을 반환한다.
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

		// 앤드 연산으로 컴포넌트 소유 여부 확인
		inline constexpr bool HasComponent(const EComponentType type) { return m_ComponentMask & GetComponentMask(type); }

		// T타입의 컴포넌트를 소유하고 있는지 확인
		template <typename T>
		bool HasComponent() { return HasComponent(IComponent::TypeToEnum<T>()); }

		// T 타입의 컴포넌트 삭제
		template <typename T>
		void RemoveComponent()
		{
			// 열거체로 변환한다.
			const EComponentType type = IComponent::TypeToEnum<T>();

			for (auto it = m_vecComponents.begin(); it != m_vecComponents.end(); it++)
			{
				auto component = *it;
				// 만약 컴포넌트를 발견했다면
				if (component->GetComponentType() == type)
				{
					// 삭제시 호출되는 함수 호출
					component->OnRemove();
					// 삭제
					it = m_vecComponents.erase(it);
					// 마스크 변수에서도 제거한다.
					m_ComponentMask &= ~GetComponentMask(type);
					break;
				}
			}

			// 이벤트 발생
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