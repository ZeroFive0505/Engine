#pragma once

#include "IComponent.h"


namespace PlayGround
{
	class Audio;

	// ������ ������Ʈ
	class AudioListener : public IComponent
	{
	public:
		AudioListener(Context* context, Entity* entity, uint64_t id = 0);
		~AudioListener() = default;

		// IComponent ���� �޼��� �������̵�
		void OnInit() override;
		void Update(double delta_time) override;

	private:
		Audio* m_Audio;
	};
}