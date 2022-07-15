#pragma once

#include <vector>
#include <memory>
#include "RHI/RHI_Definition.h"
#include "Widgets/Widget.h"

// 에디터에서 사용할 엔진 구성 요소
namespace PlayGround
{
	class Window;
	class Engine;
	class Context;
	class Renderer;
	class Profiler;
}

// 메인 에디터 클래스
class Editor
{
public:
	Editor();
	~Editor();

	void Update();

	// 컨텍스트 반환
	inline PlayGround::Context* GetContext() { return m_Context; }
	
	// T 타입의 위젯 반환
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

