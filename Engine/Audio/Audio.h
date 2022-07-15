#pragma once

#include "../Core/SubModule.h"

namespace FMOD
{
	class System;
}

namespace PlayGround
{
	// 트랜스폼과 프로파일러 전방 선언
	class Transform;
	class Profiler;

	// 오디오 클래스는 서브 모듈 클래스를 상속받는다.
	class Audio : public SubModule
	{
	public:
		Audio(Context* context);
		~Audio();

		// 서브 모듈 클래스 순사 가상 메서드 상속
		void OnInit() override;
		void Update(double delta_time) override;

		inline FMOD::System* GetSystemFMOD() const { return m_FmodSystem; }

		// 리스너의 트랜스폼 컴포넌트를 설정
		void SetListenerTransform(Transform* transform);

	private:
		// 로그 함수
		void LogErrorFmod(uint32_t error) const;

		uint32_t m_FmodResult = 0;
		uint32_t m_MaxChannels = 32;
		float m_Distance_entity = 1.0f;
		Transform* m_Listener = nullptr;
		Profiler* m_Profiler = nullptr;
		FMOD::System* m_FmodSystem = nullptr;
	};
}