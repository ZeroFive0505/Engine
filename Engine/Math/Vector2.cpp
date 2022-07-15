#include "Common.h"
#include "Vector2.h"

using namespace std;

namespace PlayGround::Math
{
	const Vector2 Vector2::Zero(0.0f, 0.0f);
	const Vector2 Vector2::One(1.0f, 1.0f);

	string Vector2::ToString() const
	{
		char buffer[256] = {};
		sprintf_s(buffer, "X:%f, Y:%f", x, y);
		return string(buffer);
	}
}
