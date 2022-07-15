#include "Common.h"
#include "AudioListener.h"
#include "../../Audio/Audio.h"

#include "../../Core/Context.h"

namespace PlayGround
{
	AudioListener::AudioListener(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
	{
		m_Audio = nullptr;
	}

	void AudioListener::OnInit()
	{
		m_Audio = GetContext()->GetSubModule<Audio>();
	}

	void AudioListener::Update(double delta_time)
	{
		if (!m_Audio)
			return;

		m_Audio->SetListenerTransform(GetTransform());
	}
}