#include "Common.h"
#include "Window.h"

#include "Context.h"
#include "Settings.h"
#include "Timer.h"
#include "EventSystem.h"
#include "../Audio/Audio.h"
#include "../Input/Input.h"
#include "../Physics/Physics.h"
#include "../Profiling/Profiler.h"
#include "../Rendering/Renderer.h"
#include "../Resource/ResourceCache.h"
#include "../Threading/Threading.h"
#include "../World/World.h"

using namespace std;
using namespace PlayGround::Math;


namespace PlayGround
{
	Engine::Engine()
	{
		m_Flags |= PhysicsMode;

		// 컨텍스트를 추가
		m_Context = make_shared<Context>();
		m_Context->m_Engine = this;

		// 컨텍스트에 다른 기능들을 추가한다.
		m_Context->AddSubModule<Settings>();
		m_Context->AddSubModule<Timer>();
		m_Context->AddSubModule<Threading>();
		m_Context->AddSubModule<Window>();
		m_Context->AddSubModule<Input>(ETickType::Smoothed);
		m_Context->AddSubModule<ResourceCache>();
		m_Context->AddSubModule<Audio>();
		m_Context->AddSubModule<Physics>();
		m_Context->AddSubModule<World>(ETickType::Smoothed);
		m_Context->AddSubModule<Profiler>();
		m_Context->AddSubModule<Renderer>();

		m_Context->OnInit();

		m_Context->OnPostInit();
	}

	Engine::~Engine()
	{
		m_Context->OnExit();

		EventSystem::Get().Clear();
	}

	void Engine::Update() const
	{
		m_Context->PrevUpdate();

		m_Context->Update(ETickType::Variable, m_Context->GetSubModule<Timer>()->GetDeltaTimeSec());
		m_Context->Update(ETickType::Smoothed, m_Context->GetSubModule<Timer>()->GetDeltaTimeSmoothedSec());

		m_Context->PostUpdate();
	}
}