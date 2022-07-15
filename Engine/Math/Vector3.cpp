#include "Common.h"

using namespace std;

namespace PlayGround::Math
{
    const Vector3 Vector3::Zero(0.0f, 0.0f, 0.0f);
    const Vector3 Vector3::One(1.0f, 1.0f, 1.0f);
    const Vector3 Vector3::Left(-1.0f, 0.0f, 0.0f);
    const Vector3 Vector3::Right(1.0f, 0.0f, 0.0f);
    const Vector3 Vector3::Up(0.0f, 1.0f, 0.0f);
    const Vector3 Vector3::Down(0.0f, -1.0f, 0.0f);
    const Vector3 Vector3::Forward(0.0f, 0.0f, 1.0f);
    const Vector3 Vector3::Backward(0.0f, 0.0f, -1.0f);
    const Vector3 Vector3::Inf(numeric_limits<float>::infinity(), numeric_limits<float>::infinity(), numeric_limits<float>::infinity());
    const Vector3 Vector3::NegInf(-numeric_limits<float>::infinity(), -numeric_limits<float>::infinity(), -numeric_limits<float>::infinity());

    Vector3::Vector3(const Vector4& value)
    {
        x = value.x;
        y = value.y;
        z = value.z;
    }

    string Vector3::ToString() const
    {
        char buffer[256] = {};
        sprintf_s(buffer, "X:%f, Y:%f, Z:%f", x, y, z);
        return string(buffer);
    }
}