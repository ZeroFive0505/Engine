#include "Common.h"
#include "Animation.h"

using namespace std;

namespace PlayGround
{
	Animation::Animation(Context* context) : IResource(context, EResourceType::Animation)
	{

	}

	bool Animation::LoadFromFile(const string& file_path)
	{
		return true;
	}

	bool Animation::SaveToFile(const string& file_path)
	{
		return true;
	}
}