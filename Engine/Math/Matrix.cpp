#include "Common.h"

#include "Matrix.h"

using namespace std;

namespace PlayGround::Math
{
    const Matrix Matrix::Identity
    (
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    string Matrix::ToString() const
    {
        char buffer[256] = {};
        sprintf_s(buffer, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33);
        return string(buffer);
    }
}