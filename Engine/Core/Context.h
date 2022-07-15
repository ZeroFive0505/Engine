#pragma once

#include "SubModule.h"
#include "../Log/Logger.h"
#include "../EngineDefinition.h"

/// <summary>
/// 하나의 컨텍스트를 이용하여 엔진의 기능을 추가, 관리한다.
/// </summary>

namespace PlayGround
{
	class Engine;
	
	// 업데이트 주기
	enum class ETickType
	{
		Variable,
		Smoothed
	};

	// 엔진에 추가되는 서브기능
	struct sSubModule
	{
		sSubModule(const std::shared_ptr<SubModule>& _submodule, ETickType _tick_group)
		{
			ptr = _submodule;
			tick_group = _tick_group;
		}

		std::shared_ptr<SubModule> ptr;
		ETickType tick_group;
	};

	class Context
	{
	public:
		Context() = default;
		
		~Context()
		{
			// 컨텍스트 소멸시에는 역으로 순환하면서 해제한다.
			// 상호의존성이 존재할 수도 있으니 반대로 반복
			size_t size = m_vecSubModules.size() - 1;

			for (size_t i = size; i > 0; i--)
			{
				m_vecSubModules[i].ptr.reset();
			}

			m_vecSubModules.clear();
		}

		// 새로운 서브모듈을 추가한다.
		template <typename T>
		void AddSubModule(ETickType tick_group = ETickType::Variable)
		{
			// T가 서브모듈을 상속받는지 확인
			Validate_SubModuleType<T>();

			m_vecSubModules.emplace_back(std::make_shared<T>(this), tick_group);
		}

		// T타입의 서브 모듈 반환
		template <typename T>
		T* GetSubModule() const
		{
			Validate_SubModuleType<T>();

			for (const auto& submodule : m_vecSubModules)
			{
				if (submodule.ptr)
				{
					if (typeid(T) == typeid(*submodule.ptr))
						return static_cast<T*>(submodule.ptr.get());
				}
			}

			return nullptr;
		}

		// 컨텍스트 초기화시에 호출
		void OnInit()
		{
			for (const auto& submodule : m_vecSubModules)
			{
				submodule.ptr->OnInit();
			}
		}
		
		// 컨텍스트 초기화 이후에 호출
		void OnPostInit()
		{
			for (const auto& submodule : m_vecSubModules)
			{
				submodule.ptr->OnPostInit();
			}
		}

		// 업데이트 이전 호출
		void PrevUpdate()
		{
			for (const auto& submodule : m_vecSubModules)
			{
				submodule.ptr->PrevUpdate();
			}
		}

		// 업데이트
		void Update(ETickType tick_group, double delta_time)
		{
			for (const auto& submodule : m_vecSubModules)
			{
				if (submodule.tick_group != tick_group)
					continue;

				submodule.ptr->Update(delta_time);
			}
		}

		// 업데이트 이후
		void PostUpdate()
		{
			for (const auto& submodule : m_vecSubModules)
			{
				submodule.ptr->PostUpdate();
			}
		}

		// 종료시
		void OnExit()
		{
			for (const auto& submodule : m_vecSubModules)
			{
				submodule.ptr->OnExit();
			}
		}

		Engine* m_Engine = nullptr;

	private:
		std::vector<sSubModule> m_vecSubModules;
	};
}