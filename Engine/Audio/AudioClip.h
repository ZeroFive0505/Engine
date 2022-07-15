#pragma once

#include "../Resource/IResource.h"
#include "../Math/Vector3.h"

// FMOD
namespace FMOD
{
	class System;
	class Sound;
	class Channel;
}

namespace PlayGround
{	
	// 트랜스폼 컴포넌트 전방 선언
	class Transform;

	// 플레이 타입
	enum EPlayMode
	{
		Play_Memory,
		Play_Stream
	};

	// 감쇠
	enum ERollOff
	{
		Linear,
		Custom
	};
	
	// 오디오 클립 클래스 IResource을 상속받는다.
	class AudioClip : public IResource
	{
	public:
		AudioClip(Context* context);
		~AudioClip();

		// 경로에서 불러온다.
		bool LoadFromFile(const std::string& file_path) override;
		bool SaveToFile(const std::string& file_path) override;

		// 오디오 재생
		bool Play();
		// 오디오 일시정지
		bool Pause();
		// 오디오 정지
		bool Stop();

		// 오디오 반복
		bool SetLoop(bool loop);

		// 볼륨 설정
		bool SetVolume(float volume);

		// 음소거
		bool SetMute(bool mute);

		// 우선순위
		bool SetPriority(int priority);

		// 피치 설정
		bool SetPitch(float pitch);

		// 패닝 설정
		bool SetPan(float pan);

		// 감쇠 설정
		bool SetRollOff(const std::vector<Math::Vector3>& curve_points);
		bool SetRollOff(const ERollOff rolloff);

		// 트랜스폼 컴포넌트 설정
		inline void SetTransform(Transform* transform) { m_Transform = transform; }

		// 업데이트
		bool Update();

		// 플레이 여부 확인
		bool IsPlaying();

	private:
		// 사운드 생성
		bool CreateSound(const std::string& file_path);
		bool CreateStream(const std::string& file_path);

		// 사운드 모드를 가져온다.
		int GetSoundMode() const;
		// 로그 출력
		void LogErrorFmod(int error) const;
		// 채널 유효여부 확인
		bool IsChannelValid() const;

		// 트랜스폼 컴포넌트
		Transform* m_Transform;
		// FMOD 시스템
		FMOD::System* m_SystemFMOD;
		// FMOD 사운드
		FMOD::Sound* m_SoundFMOD;
		// FMOD 채널
		FMOD::Channel* m_ChannelFMOD;
		// 플레이 모드
		EPlayMode m_PlayMode;
		int m_ModeLoop;
		// 최대, 최소 거리
		float m_MinDistance;
		float m_MaxDistance;
		int m_ModeRollOff;
		int m_Result;
	};
}
