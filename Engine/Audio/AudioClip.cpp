#include "Common.h"
#include "AudioClip.h"
#include <fmod.hpp>
#include <fmod_errors.h>
#include "Audio.h"
#include "../World/Components/Transform.h"
#include "../IO/FileStream.h"

using namespace std;
using namespace PlayGround::Math;
using namespace FMOD;

namespace PlayGround
{
	AudioClip::AudioClip(Context* context) : IResource(context, EResourceType::Audio)
	{
		// 오디오 클립 초기화
		m_Transform = nullptr;
		// 컨텍스트에서 오디오 모듈을 가져온다.
		m_SystemFMOD = static_cast<System*>(context->GetSubModule<Audio>()->GetSystemFMOD());
		// FMOD 결과 값
		m_Result = FMOD_OK;
		m_SoundFMOD = nullptr;
		m_ChannelFMOD = nullptr;
		m_PlayMode = Play_Memory;
		m_MinDistance = 1.0f;
		m_MaxDistance = 10000.0f;
		m_ModeRollOff = FMOD_3D_LINEARROLLOFF;
		m_ModeLoop = FMOD_LOOP_OFF;
	}

	AudioClip::~AudioClip()
	{
		if (!m_SoundFMOD)
			return;

		m_Result = m_SoundFMOD->release();

		if (m_Result != FMOD_OK)
			LogErrorFmod(m_Result);
	}

	bool AudioClip::LoadFromFile(const string& file_path)
	{
		m_SoundFMOD = nullptr;
		m_ChannelFMOD = nullptr;

		if (FileSystem::GetExtensionFromFilePath(file_path) == EXTENSION_AUDIO)
		{
			auto file = make_unique<FileStream>(file_path, FileStream_Read);

			if (!file->IsOpen())
				return false;

			SetResourceFilePath(file->ReadAs<string>());

			file->Close();
		}
		else
			SetResourceFilePath(file_path);

		return (m_PlayMode == Play_Memory) ? CreateSound(GetResourceFilePath()) : CreateStream(GetResourceFilePath());
	}

	bool AudioClip::SaveToFile(const string& file_path)
	{
		auto file = make_unique<FileStream>(file_path, FileStream_Write);
		if (!file->IsOpen())
			return false;

		file->Write(GetResourceFilePath());

		file->Close();

		return true;
	}

	bool AudioClip::Play()
	{
		if (IsChannelValid())
		{
			bool is_playing = false;
			m_Result = m_ChannelFMOD->isPlaying(&is_playing);

			if (m_Result != FMOD_OK)
			{
				LogErrorFmod(m_Result);
				return false;
			}

			if (is_playing)
				return true;
		}

		m_Result = m_SystemFMOD->playSound(m_SoundFMOD, nullptr, false, &m_ChannelFMOD);

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::Pause()
	{
		if (!IsChannelValid())
			return true;

		bool is_paused = false;

		m_Result = m_ChannelFMOD->getPaused(&is_paused);

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		if (!is_paused)
			return true;

		m_Result = m_ChannelFMOD->setPaused(true);

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::Stop()
	{
		if (!IsChannelValid())
			return true;

		if (!IsPlaying())
			return true;

		m_Result = m_ChannelFMOD->stop();

		if (m_Result != FMOD_OK)
		{
			m_ChannelFMOD = nullptr;
			LogErrorFmod(m_Result);
			return false;
		}

		m_ChannelFMOD = nullptr;
		
		return true;
	}

	bool AudioClip::SetLoop(const bool loop)
	{
		m_ModeLoop = loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

		if (!m_SoundFMOD)
			return false;

		if (loop)
			m_SoundFMOD->setLoopCount(-1);


		m_Result = m_SoundFMOD->setMode(GetSoundMode());

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::SetVolume(float volume)
	{
		if (!IsChannelValid())
			return false;

		m_Result = m_ChannelFMOD->setVolume(volume);

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::SetMute(const bool mute)
	{
		if (!IsChannelValid())
			return false;

		m_Result = m_ChannelFMOD->setMute(mute);

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::SetPriority(const int priority)
	{
		if (!IsChannelValid())
			return false;

		m_Result = m_ChannelFMOD->setPriority(priority);
		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::SetPitch(const float pitch)
	{
		if (!IsChannelValid())
			return false;

		m_Result = m_ChannelFMOD->setPitch(pitch);
		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::SetPan(const float pan)
	{
		if (!IsChannelValid())
			return false;

		m_Result = m_ChannelFMOD->setPan(pan);
		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::SetRollOff(const vector<Vector3>& curve_points)
	{
		if (!IsChannelValid())
			return false;

		SetRollOff(Custom);

		vector<FMOD_VECTOR> fmod_curve;

		for (const auto& point : curve_points)
		{
			fmod_curve.push_back(FMOD_VECTOR{ point.x, point.y, point.z });
		}

		m_Result = m_ChannelFMOD->set3DCustomRolloff(&fmod_curve.front(), static_cast<int>(fmod_curve.size()));

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::SetRollOff(const ERollOff rolloff)
	{
		switch (rolloff)
		{
		case PlayGround::Linear:
			m_ModeRollOff = FMOD_3D_LINEARROLLOFF;
			break;
		case PlayGround::Custom:
			m_ModeRollOff = FMOD_3D_CUSTOMROLLOFF;
			break;
		default:
			break;
		}

		return true;
	}

	bool AudioClip::Update()
	{
		if (!IsChannelValid() || !m_Transform)
			return true;

		// 트랜스폼의 위치 정보를 기반으로 3D 사운드를 설정한다.
		const Vector3 pos = m_Transform->GetPosition();

		FMOD_VECTOR fmod_pos = { pos.x, pos.y, pos.z };
		FMOD_VECTOR fmod_vel = { 0.0f, 0.0f, 0.0f };

		m_Result = m_ChannelFMOD->set3DAttributes(&fmod_pos, &fmod_vel);

		if (m_Result != FMOD_OK)
		{
			m_ChannelFMOD = nullptr;
			LogErrorFmod(m_Result);
			return false;
		}

		return true;

	}

	bool AudioClip::IsPlaying()
	{
		if (!IsChannelValid())
			return false;

		bool is_playing = false;

		m_Result = m_ChannelFMOD->isPlaying(&is_playing);
		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return is_playing;
	}

	bool AudioClip::CreateSound(const string& file_path)
	{
		m_Result = m_SystemFMOD->createSound(file_path.c_str(), GetSoundMode(), nullptr, &m_SoundFMOD);

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		m_Result = m_SoundFMOD->set3DMinMaxDistance(m_MinDistance, m_MaxDistance);

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	bool AudioClip::CreateStream(const string& file_path)
	{
		m_Result = m_SystemFMOD->createStream(file_path.c_str(), GetSoundMode(), nullptr, &m_SoundFMOD);

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		m_Result = m_SoundFMOD->set3DMinMaxDistance(m_MinDistance, m_MaxDistance);

		if (m_Result != FMOD_OK)
		{
			LogErrorFmod(m_Result);
			return false;
		}

		return true;
	}

	int AudioClip::GetSoundMode() const
	{
		return FMOD_3D | m_ModeLoop | m_ModeRollOff;
	}

	void AudioClip::LogErrorFmod(int error) const
	{
		LOG_ERROR("%s", FMOD_ErrorString(static_cast<FMOD_RESULT>(error)));
	}

	bool AudioClip::IsChannelValid() const
	{
		if (!m_ChannelFMOD)
			return false;

		bool value;

		return m_ChannelFMOD->isPlaying(&value) == FMOD_OK;
	}
}