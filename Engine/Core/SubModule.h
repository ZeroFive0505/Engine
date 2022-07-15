#pragma once

#include <type_traits>
#include <memory>
#include "../EngineDefinition.h"

namespace PlayGround
{
	class Context;
	// 엔진에 추가되는 서브 모듈
	class SubModule : public std::enable_shared_from_this<SubModule>
	{
	public:
		SubModule(Context* context)
		{
			m_Context = context;
		}

		// 서브 모듈 클래스의 가상함수들 

		virtual ~SubModule() = default;

		virtual void OnInit() {}

		virtual void OnPostInit() {}

		virtual void PrevUpdate() {}

		virtual void Update(double delta_time) {}

		virtual void PostUpdate() {}

		virtual void OnExit() {}

		// T로 타입 변환 후 반환
		template <typename T>
		std::shared_ptr<T> GetSharedPtr()
		{
			return std::dynamic_pointer_cast<T>(shared_from_this());
		}

	protected:
		Context* m_Context;
	};

	// T가 서브 모듈 클래스를 상속받는지 체크한다.
	template <typename T>
	constexpr void Validate_SubModuleType()
	{
		static_assert(std::is_base_of<SubModule, T>::value, "Provided type does not implement ISubModule");
	}
}