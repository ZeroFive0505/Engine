#pragma once

#include "SubModule.h"
#include "../Log/Logger.h"
#include "../EngineDefinition.h"

/// <summary>
/// �ϳ��� ���ؽ�Ʈ�� �̿��Ͽ� ������ ����� �߰�, �����Ѵ�.
/// </summary>

namespace PlayGround
{
	class Engine;
	
	// ������Ʈ �ֱ�
	enum class ETickType
	{
		Variable,
		Smoothed
	};

	// ������ �߰��Ǵ� ������
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
			// ���ؽ�Ʈ �Ҹ�ÿ��� ������ ��ȯ�ϸ鼭 �����Ѵ�.
			// ��ȣ�������� ������ ���� ������ �ݴ�� �ݺ�
			size_t size = m_vecSubModules.size() - 1;

			for (size_t i = size; i > 0; i--)
			{
				m_vecSubModules[i].ptr.reset();
			}

			m_vecSubModules.clear();
		}

		// ���ο� �������� �߰��Ѵ�.
		template <typename T>
		void AddSubModule(ETickType tick_group = ETickType::Variable)
		{
			// T�� �������� ��ӹ޴��� Ȯ��
			Validate_SubModuleType<T>();

			m_vecSubModules.emplace_back(std::make_shared<T>(this), tick_group);
		}

		// TŸ���� ���� ��� ��ȯ
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

		// ���ؽ�Ʈ �ʱ�ȭ�ÿ� ȣ��
		void OnInit()
		{
			for (const auto& submodule : m_vecSubModules)
			{
				submodule.ptr->OnInit();
			}
		}
		
		// ���ؽ�Ʈ �ʱ�ȭ ���Ŀ� ȣ��
		void OnPostInit()
		{
			for (const auto& submodule : m_vecSubModules)
			{
				submodule.ptr->OnPostInit();
			}
		}

		// ������Ʈ ���� ȣ��
		void PrevUpdate()
		{
			for (const auto& submodule : m_vecSubModules)
			{
				submodule.ptr->PrevUpdate();
			}
		}

		// ������Ʈ
		void Update(ETickType tick_group, double delta_time)
		{
			for (const auto& submodule : m_vecSubModules)
			{
				if (submodule.tick_group != tick_group)
					continue;

				submodule.ptr->Update(delta_time);
			}
		}

		// ������Ʈ ����
		void PostUpdate()
		{
			for (const auto& submodule : m_vecSubModules)
			{
				submodule.ptr->PostUpdate();
			}
		}

		// �����
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