#include "Common.h"
#include "EngineObject.h"

namespace PlayGround
{
	uint64_t GID = 0;

	EngineObject::EngineObject(Context* context /*= nullptr*/)
	{
		m_Context = context;
		m_ObjectID = GenerateObjectID();
	}
}
