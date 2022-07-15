#pragma once

#include "IComponent.h"
#include <memory>
#include <string>

namespace PlayGround
{
	class AudioClip;

	// 오디오 소스 컴포넌트
	class AudioSource : public IComponent
	{
	public:
		AudioSource(Context* context, Entity* entity, uint64_t id = 0);
		~AudioSource() = default;

		// IComponent 가상 메서드 오버라이드
		void OnInit() override;
		void OnStart() override;
		void OnStop() override;
		void OnRemove() override;
		void Update(double delta_time) override;
		void Serialize(FileStream* stream) override;
		void Deserialize(FileStream* stream) override;

		void SetAudioClip(const std::string& file_path);
		std::string GetAudioClipName() const;

		bool Play() const;
		bool Stop() const;

		inline bool GetMute() const { return m_Mute; }
		void SetMute(bool mute);

		inline bool GetPlayOnStart() const { return m_PlayOnStart; }
		inline void SetPlayOnStart(const bool play_on_start) { m_PlayOnStart = play_on_start; }

		inline bool GetLoop() const { return m_Loop; }
		inline void SetLoop(const bool loop) { m_Loop = loop; }

		inline int GetPriority() const { return m_Priority; }
		void SetPriority(int priority);

		inline float GetVolume() const { return m_Volume; }
		void SetVolume(float volume);

		inline float GetPitch() const { return m_Pitch; }
		void SetPitch(float pitch);

		inline float GetPan() const { return m_Pan; }
		void SetPan(float pan);

	private:
		std::shared_ptr<AudioClip> m_AudioClip;
		bool m_Mute;
		bool m_PlayOnStart;
		bool m_Loop;
		int m_Priority;
		float m_Volume;
		float m_Pitch;
		float m_Pan;
		bool m_Audio_clip_loaded;
	};
}

