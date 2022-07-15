#include "Common.h"
#include "Quaternion.h"
#include "Matrix.h"

using namespace std;

namespace PlayGround::Math
{
	const Quaternion Quaternion::Identity(0.0f, 0.0f, 0.0f, 1.0f);

	void Quaternion::FromAxes(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis)
	{
		*this = Matrix(
			xAxis.x, yAxis.x, zAxis.x, 0.0f,
			xAxis.y, yAxis.y, zAxis.y, 0.0f,
			xAxis.z, yAxis.z, zAxis.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		).GetRotation();
	}

	string Quaternion::ToString() const
	{
		char buffer[256] = {};
		sprintf_s(buffer, "X:%f, Y:%f, Z:%f, W:%f", x, y, z, w);
		return string(buffer);
	}
}