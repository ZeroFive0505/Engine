#pragma once

#include "IComponent.h"


namespace PlayGround
{
	class Audio;

	// 리스너 컴포넌트
	class AudioListener : public IComponent
	{
	public:
		AudioListener(Context* context, Entity* entity, uint64_t id = 0);
		~AudioListener() = default;

		// IComponent 가상 메서드 오버라이드
		void OnInit() override;
		void Update(double delta_time) override;

	private:
		Audio* m_Audio;
	};
}