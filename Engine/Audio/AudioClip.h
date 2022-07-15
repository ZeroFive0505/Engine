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
	// Ʈ������ ������Ʈ ���� ����
	class Transform;

	// �÷��� Ÿ��
	enum EPlayMode
	{
		Play_Memory,
		Play_Stream
	};

	// ����
	enum ERollOff
	{
		Linear,
		Custom
	};
	
	// ����� Ŭ�� Ŭ���� IResource�� ��ӹ޴´�.
	class AudioClip : public IResource
	{
	public:
		AudioClip(Context* context);
		~AudioClip();

		// ��ο��� �ҷ��´�.
		bool LoadFromFile(const std::string& file_path) override;
		bool SaveToFile(const std::string& file_path) override;

		// ����� ���
		bool Play();
		// ����� �Ͻ�����
		bool Pause();
		// ����� ����
		bool Stop();

		// ����� �ݺ�
		bool SetLoop(bool loop);

		// ���� ����
		bool SetVolume(float volume);

		// ���Ұ�
		bool SetMute(bool mute);

		// �켱����
		bool SetPriority(int priority);

		// ��ġ ����
		bool SetPitch(float pitch);

		// �д� ����
		bool SetPan(float pan);

		// ���� ����
		bool SetRollOff(const std::vector<Math::Vector3>& curve_points);
		bool SetRollOff(const ERollOff rolloff);

		// Ʈ������ ������Ʈ ����
		inline void SetTransform(Transform* transform) { m_Transform = transform; }

		// ������Ʈ
		bool Update();

		// �÷��� ���� Ȯ��
		bool IsPlaying();

	private:
		// ���� ����
		bool CreateSound(const std::string& file_path);
		bool CreateStream(const std::string& file_path);

		// ���� ��带 �����´�.
		int GetSoundMode() const;
		// �α� ���
		void LogErrorFmod(int error) const;
		// ä�� ��ȿ���� Ȯ��
		bool IsChannelValid() const;

		// Ʈ������ ������Ʈ
		Transform* m_Transform;
		// FMOD �ý���
		FMOD::System* m_SystemFMOD;
		// FMOD ����
		FMOD::Sound* m_SoundFMOD;
		// FMOD ä��
		FMOD::Channel* m_ChannelFMOD;
		// �÷��� ���
		EPlayMode m_PlayMode;
		int m_ModeLoop;
		// �ִ�, �ּ� �Ÿ�
		float m_MinDistance;
		float m_MaxDistance;
		int m_ModeRollOff;
		int m_Result;
	};
}
