#pragma once

#include "../Core/SubModule.h"

namespace FMOD
{
	class System;
}

namespace PlayGround
{
	// Ʈ�������� �������Ϸ� ���� ����
	class Transform;
	class Profiler;

	// ����� Ŭ������ ���� ��� Ŭ������ ��ӹ޴´�.
	class Audio : public SubModule
	{
	public:
		Audio(Context* context);
		~Audio();

		// ���� ��� Ŭ���� ���� ���� �޼��� ���
		void OnInit() override;
		void Update(double delta_time) override;

		inline FMOD::System* GetSystemFMOD() const { return m_FmodSystem; }

		// �������� Ʈ������ ������Ʈ�� ����
		void SetListenerTransform(Transform* transform);

	private:
		// �α� �Լ�
		void LogErrorFmod(uint32_t error) const;

		uint32_t m_FmodResult = 0;
		uint32_t m_MaxChannels = 32;
		float m_Distance_entity = 1.0f;
		Transform* m_Listener = nullptr;
		Profiler* m_Profiler = nullptr;
		FMOD::System* m_FmodSystem = nullptr;
	};
}