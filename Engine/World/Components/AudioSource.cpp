#include "Common.h"
#include "AudioSource.h"
#include "../../Audio/AudioClip.h"
#include "../../IO/FileStream.h"
#include "../../Resource/ResourceCache.h"


using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	AudioSource::AudioSource(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
	{
		m_Mute = false;
		m_PlayOnStart = true;
		m_Loop = false;
		m_Priority = 128;
		m_Volume = 1.0f;
		m_Pitch = 1.0f;
		m_Pan = 0.0f;
		m_Audio_clip_loaded = false;
	}

	void AudioSource::OnInit()
	{
		if (!m_AudioClip)
			return;

		m_AudioClip->SetTransform(GetTransform());
	}

	void AudioSource::OnStart()
	{
		if (!m_PlayOnStart)
			return;

		Play();
	}

	void AudioSource::OnStop()
	{
		Stop();
	}

	void AudioSource::OnRemove()
	{
		if (!m_AudioClip)
			return;
		m_AudioClip->Stop();
	}

	void AudioSource::Update(double delta_time)
	{
		if (!m_AudioClip)
			return;

		m_AudioClip->Update();
	}

	void AudioSource::Serialize(FileStream* stream)
	{
		stream->Write(m_Mute);
		stream->Write(m_PlayOnStart);
		stream->Write(m_Loop);
		stream->Write(m_Priority);
		stream->Write(m_Volume);
		stream->Write(m_Pitch);
		stream->Write(m_Pan);

		const bool has_audio_clip = m_AudioClip != nullptr;
		stream->Write(has_audio_clip);
		if (has_audio_clip)
		{
			stream->Write(m_AudioClip->GetResourceName());
		}
	}

	void AudioSource::Deserialize(FileStream* stream)
	{
		stream->Read(&m_Mute);
		stream->Read(&m_PlayOnStart);
		stream->Read(&m_Loop);
		stream->Read(&m_Priority);
		stream->Read(&m_Volume);
		stream->Read(&m_Pitch);
		stream->Read(&m_Pan);

		if (stream->ReadAs<bool>())
		{
			m_AudioClip = m_Context->GetSubModule<ResourceCache>()->GetByName<AudioClip>(stream->ReadAs<string>());
		}
	}

	void AudioSource::SetAudioClip(const string& file_path)
	{
		auto audio_clip = make_shared<AudioClip>(m_Context);
		if (audio_clip->LoadFromFile(file_path))
		{
			m_AudioClip = m_Context->GetSubModule<ResourceCache>()->Cache(audio_clip);
		}
	}

	string AudioSource::GetAudioClipName() const
	{
		return m_AudioClip ? m_AudioClip->GetResourceName() : "";
	}

	bool AudioSource::Play() const
	{
		if (!m_AudioClip)
			return false;

		m_AudioClip->Play();
		m_AudioClip->SetMute(m_Mute);
		m_AudioClip->SetVolume(m_Volume);
		m_AudioClip->SetLoop(m_Loop);
		m_AudioClip->SetPriority(m_Priority);
		m_AudioClip->SetPan(m_Pan);

		return true;
	}

	bool AudioSource::Stop() const
	{
		if (!m_AudioClip)
			return false;

		return m_AudioClip->Stop();
	}

	void AudioSource::SetMute(bool mute)
	{
		if (m_Mute == mute || !m_AudioClip)
			return;

		m_Mute = mute;
		m_AudioClip->SetMute(mute);
	}

	void AudioSource::SetPriority(int priority)
	{
		if (!m_AudioClip)
			return;

		m_Priority = static_cast<int>(Util::Clamp(0, 255, priority));
		m_AudioClip->SetPriority(m_Priority);
	}

	void AudioSource::SetVolume(float volume)
	{
		if (!m_AudioClip)
			return;

		m_Volume = Util::Clamp(0.0f, 1.0f, volume);
		m_AudioClip->SetVolume(m_Volume);
	}

	void AudioSource::SetPitch(float pitch)
	{
		if (!m_AudioClip)
			return;

		m_Pitch = Util::Clamp(0.0f, 3.0f, pitch);
		m_AudioClip->SetPitch(m_Pitch);
	}

	void AudioSource::SetPan(float pan)
	{
		if (!m_AudioClip)
			return;

		m_Pan = Util::Clamp(-1.0f, 1.0f, pan);
		m_AudioClip->SetPan(m_Pan);
	}
}