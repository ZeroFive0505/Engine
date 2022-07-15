#pragma once

#include <type_traits>
#include <memory>
#include "../EngineDefinition.h"

namespace PlayGround
{
	class Context;
	// ������ �߰��Ǵ� ���� ���
	class SubModule : public std::enable_shared_from_this<SubModule>
	{
	public:
		SubModule(Context* context)
		{
			m_Context = context;
		}

		// ���� ��� Ŭ������ �����Լ��� 

		virtual ~SubModule() = default;

		virtual void OnInit() {}

		virtual void OnPostInit() {}

		virtual void PrevUpdate() {}

		virtual void Update(double delta_time) {}

		virtual void PostUpdate() {}

		virtual void OnExit() {}

		// T�� Ÿ�� ��ȯ �� ��ȯ
		template <typename T>
		std::shared_ptr<T> GetSharedPtr()
		{
			return std::dynamic_pointer_cast<T>(shared_from_this());
		}

	protected:
		Context* m_Context;
	};

	// T�� ���� ��� Ŭ������ ��ӹ޴��� üũ�Ѵ�.
	template <typename T>
	constexpr void Validate_SubModuleType()
	{
		static_assert(std::is_base_of<SubModule, T>::value, "Provided type does not implement ISubModule");
	}
}