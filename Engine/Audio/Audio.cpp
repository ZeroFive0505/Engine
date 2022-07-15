#include "Common.h"
#include "Audio.h"
#include "../Profiling/Profiler.h"
#include "../World/Components/Transform.h"
#include <fmod.hpp>
#include <fmod_errors.h>

#include "../Core/EventSystem.h"
#include "../Core/Context.h"
#include "../Core/SubModule.h"
#include "../Core/Engine.h"
#include "../Core/Settings.h"

using namespace std;
using namespace FMOD;

namespace PlayGround
{
	Audio::Audio(Context* context) : SubModule(context)
	{

	}

	Audio::~Audio()
	{
		// 오디오 클래스 소멸시 WorldClear 이벤트의 구독을 해제한다.
		UNSUBSCRIBE_FROM_EVENT(EventType::WorldClear, [this](Variant) {
			m_Listener = nullptr;
		});

		if (!m_FmodSystem)
			return;

		m_FmodResult = m_FmodSystem->close();

		if (m_FmodResult != FMOD_OK)
		{
			LogErrorFmod(m_FmodResult);
			return;
		}

		m_FmodResult = m_FmodSystem->release();

		if (m_FmodResult != FMOD_OK)
			LogErrorFmod(m_FmodResult);
	}

	void Audio::OnInit()
	{
		m_FmodResult = System_Create(&m_FmodSystem);

		if (m_FmodResult != FMOD_OK)
		{
			LogErrorFmod(m_FmodResult);
			ASSERT(0 && "Failed to create FMOD instance");
		}

		uint32_t version;
		m_FmodResult = m_FmodSystem->getVersion(&version);

		if (m_FmodResult != FMOD_OK)
		{
			LogErrorFmod(m_FmodResult);
			ASSERT(0 && "Failed to get FMOD version");
		}

		int driver_count = 0;
		m_FmodResult = m_FmodSystem->getNumDrivers(&driver_count);
		if (m_FmodResult != FMOD_OK)
		{
			LogErrorFmod(m_FmodResult);
			ASSERT(0 && "Failed to get a sound device");
		}

		m_FmodResult = m_FmodSystem->init(m_MaxChannels, FMOD_INIT_NORMAL, nullptr);
		if (m_FmodResult != FMOD_OK)
		{
			LogErrorFmod(m_FmodResult);
			ASSERT(0 && "Failed to initialize FMOD");
		}

		m_FmodResult = m_FmodSystem->set3DSettings(1.0f, m_Distance_entity, 0.0f);
		if (m_FmodResult != FMOD_OK)
		{
			LogErrorFmod(m_FmodResult);
			ASSERT(0 && "Failed to set 3D settings");
		}

		stringstream ss;
		ss << hex << version;
		const std::string major = ss.str().erase(1, 4);
		const std::string minor = ss.str().erase(0, 1).erase(2, 2);
		const std::string rev = ss.str().erase(0, 3);
		
		m_Context->GetSubModule<Settings>()->RegisterThirdParty("FMOD", major + "." + minor + "." + rev, "https://www.fmod.com/download");

		m_Profiler = m_Context->GetSubModule<Profiler>();

		// 오디오 생성시 WorldClear 이벤트를 구독한다.
		SUBSCRIBE_TO_EVENT(EventType::WorldClear, [this](Variant) {
			m_Listener = nullptr;
		});
	}

	void Audio::Update(double delta_time)
	{
		// 만약 게임 플레이 모드가 아니라면 그냥 반환
		if (!m_Context->m_Engine->IsEngineModeSet(GameMode))
			return;

		// 프로파일링 시작
		SCOPED_TIME_BLOCK(m_Profiler);

		m_FmodResult = m_FmodSystem->update();

		if (m_FmodResult != FMOD_OK)
		{
			LogErrorFmod(m_FmodResult);
			return;
		}


		// 리스너를 업데이트 한다.
		if (m_Listener)
		{
			auto position = m_Listener->GetPosition();
			auto velocity = Math::Vector3::Zero;
			auto forward = m_Listener->GetForward();
			auto up = m_Listener->GetUp();

			m_FmodResult = m_FmodSystem->set3DListenerAttributes(
				0,
				reinterpret_cast<FMOD_VECTOR*>(&position),
				reinterpret_cast<FMOD_VECTOR*>(&velocity),
				reinterpret_cast<FMOD_VECTOR*>(&forward),
				reinterpret_cast<FMOD_VECTOR*>(&up)
			);

			if (m_FmodResult != FMOD_OK)
			{
				LogErrorFmod(m_FmodResult);
				return;
			}
		}
	}

	void Audio::SetListenerTransform(Transform* transform)
	{
		m_Listener = transform;
	}

	void Audio::LogErrorFmod(uint32_t error) const
	{
		LOG_ERROR("%s", FMOD_ErrorString(static_cast<FMOD_RESULT>(error)));
	}
}