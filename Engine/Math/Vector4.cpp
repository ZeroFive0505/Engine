#include "Common.h"
#include "Vector4.h"
#include "Vector3.h"

using namespace std;

namespace PlayGround::Math
{
    const Vector4 Vector4::One(1.0f, 1.0f, 1.0f, 1.0f);
    const Vector4 Vector4::Zero(0.0f, 0.0f, 0.0f, 0.0f);
    const Vector4 Vector4::Inf(numeric_limits<float>::infinity(), numeric_limits<float>::infinity(), numeric_limits<float>::infinity(), numeric_limits<float>::infinity());
    const Vector4 Vector4::NegInf(-numeric_limits<float>::infinity(), -numeric_limits<float>::infinity(), -numeric_limits<float>::infinity(), -numeric_limits<float>::infinity());

    Vector4::Vector4(const Vector3& value, float _w)
    {
        x = value.x;
        y = value.y;
        z = value.z;
        w = _w;
    }

    Vector4::Vector4(const Vector3& value)
    {
        x = value.x;
        y = value.y;
        z = value.z;
        w = 0.0f;
    }


    string Vector4::ToString() const
    {
        char buffer[256] = {};
        sprintf_s(buffer, "X:%f, Y:%f, Z:%f, W:%f", x, y, z, w);
        return string(buffer);
    }
}