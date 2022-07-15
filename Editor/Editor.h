#pragma once

#include <vector>
#include <memory>
#include "RHI/RHI_Definition.h"
#include "Widgets/Widget.h"

// �����Ϳ��� ����� ���� ���� ���
namespace PlayGround
{
	class Window;
	class Engine;
	class Context;
	class Renderer;
	class Profiler;
}

// ���� ������ Ŭ����
class Editor
{
public:
	Editor();
	~Editor();

	void Update();

	// ���ؽ�Ʈ ��ȯ
	inline PlayGround::Context* GetContext() { return m_Context; }
	
	// T Ÿ���� ���� ��ȯ
	template <typename T>
	T* GetWidget()
	{
		for (const auto& widget : m_vecWidgets)
		{
			if (T* widget_t = dynamic_cast<T*>(widget.get()))
				return widget_t;
		}

		return nullptr;
	}

private:
	void Initialize();
	void BeginWindow();

	bool m_Editor_begun = false;
	std::unique_ptr<PlayGround::Engine> m_Engine;
	std::vector<std::shared_ptr<Widget>> m_vecWidgets;
	PlayGround::Context* m_Context = nullptr;
};

