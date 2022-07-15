#pragma once

#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Math/Quaternion.h"

#include "LinearMath/btQuaternion.h"


// Bullet 라이브러리에서 엔진 수학 라이브러리 변환 유틸리티 함수들
inline PlayGround::Math::Vector3 ToVector3(const btVector3& vector)
{
    return PlayGround::Math::Vector3(vector.getX(), vector.getY(), vector.getZ());
}

inline PlayGround::Math::Vector4 ToVector4(const btVector3& vector)
{
    return PlayGround::Math::Vector4(vector.getX(), vector.getY(), vector.getZ(), 1.0f);
}


// 엔진 수학 라이브러리에서 Bullet 라이브러리 유틸리니 함수들
inline btVector3 ToBtVector3(const PlayGround::Math::Vector3& vector)
{
    return btVector3(vector.x, vector.y, vector.z);
}

inline btQuaternion ToBtQuaternion(const PlayGround::Math::Quaternion& quaternion)
{
    return btQuaternion(quaternion.x, quaternion.y, quaternion.z, quaternion.w);
}

inline PlayGround::Math::Quaternion ToQuaternion(const btQuaternion& quaternion)
{
    return PlayGround::Math::Quaternion(quaternion.getX(), quaternion.getY(), quaternion.getZ(), quaternion.getW());
}
