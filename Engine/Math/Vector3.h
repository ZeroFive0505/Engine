#pragma once

#include <string>
#include "MathUtil.h"
#include "../EngineDefinition.h"

namespace PlayGround::Math
{
	class Vector4;

	class Vector3
	{
    public:
        Vector3()
        {
            x = 0;
            y = 0;
            z = 0;
        }

        Vector3(const Vector3& vector)
        {
            x = vector.x;
            y = vector.y;
            z = vector.z;
        }

        Vector3(const Vector4& vector);

        Vector3(float x, float y, float z)
        {
            this->x = x;
            this->y = y;
            this->z = z;
        }

        Vector3(float pos[3])
        {
            this->x = pos[0];
            this->y = pos[1];
            this->z = pos[2];
        }

        Vector3(float f)
        {
            x = f;
            y = f;
            z = f;
        }

        void Normalize()
        {
            const float length_squared = LengthSquared();
            if (!Util::Equals(length_squared, 1.0f) && length_squared > 0.0f)
            {
                const auto length_inverted = 1.0f / Util::Sqrt(length_squared);
                x *= length_inverted;
                y *= length_inverted;
                z *= length_inverted;
            }
        };

        Vector3 Normalized() const
        {
            Vector3 v = *this;
            v.Normalize();
            return v;
        }

        static inline Vector3 Normalize(const Vector3& v) { return v.Normalized(); }

        inline bool IsNormalized() const
        {
            static const float THRESH_VECTOR_NORMALIZED = 0.01f;
            return (Util::Abs(1.f - LengthSquared()) < THRESH_VECTOR_NORMALIZED);
        }

        inline float Max()
        {
            return Util::Max3(x, y, z);
        }

        static inline float Dot(const Vector3& v1, const Vector3& v2) { return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z); }

        inline float Dot(const Vector3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }

        static inline Vector3 Cross(const Vector3& v1, const Vector3& v2)
        {
            return Vector3(
                v1.y * v2.z - v2.y * v1.z,
                -(v1.x * v2.z - v2.x * v1.z),
                v1.x * v2.y - v2.x * v1.y
            );
        }

        inline Vector3 Cross(const Vector3& v2) const { return Cross(*this, v2); }

        inline float Length() const { return Util::Sqrt(x * x + y * y + z * z); }
     
        inline float LengthSquared() const { return x * x + y * y + z * z; }

        inline void ClampMagnitude(float maxLength)
        {
            const float sqrmag = LengthSquared();

            if (sqrmag > maxLength * maxLength)
            {
                const float mag = Util::Sqrt(sqrmag);

                const float normalized_x = x / mag;
                const float normalized_y = y / mag;
                const float normalized_z = z / mag;

                x = normalized_x * maxLength;
                y = normalized_y * maxLength;
                z = normalized_z * maxLength;
            }
        }

        inline void FindBestAxisVectors(Vector3& Axis1, Vector3& Axis2) const
        {
            const float NX = Util::Abs(x);
            const float NY = Util::Abs(y);
            const float NZ = Util::Abs(z);

            if (NZ > NX && NZ > NY)	Axis1 = Vector3(1, 0, 0);
            else                    Axis1 = Vector3(0, 0, 1);

            Axis1 = (Axis1 - *this * (Axis1.Dot(*this))).Normalized();
            Axis2 = Axis1.Cross(*this);
        }

        inline float Distance(const Vector3& x) { return ((*this) - x).Length(); }
        inline float DistanceSquared(const Vector3& x) { return ((*this) - x).LengthSquared(); }
        static inline float Distance(const Vector3& a, const Vector3& b) { return (b - a).Length(); }
        static inline float DistanceSquared(const Vector3& a, const Vector3& b) { return (b - a).LengthSquared(); }

        void Floor()
        {
            x = floor(x);
            y = floor(y);
            z = floor(z);
        }

        inline Vector3 Abs() const { return Vector3(Util::Abs(x), Util::Abs(y), Util::Abs(z)); }

        inline Vector3 Lerp(const Vector3& v, float t)                                    const { return *this * (1.0f - t) + v * t; }
        static inline Vector3 Lerp(const Vector3& a, const Vector3& b, const float t) { return a + (b - a) * t; }

        Vector3 operator*(const Vector3& b) const
        {
            return Vector3(
                x * b.x,
                y * b.y,
                z * b.z
            );
        }

        void operator*=(const Vector3& b)
        {
            x *= b.x;
            y *= b.y;
            z *= b.z;
        }

        Vector3 operator*(const float value) const
        {
            return Vector3(
                x * value,
                y * value,
                z * value
            );
        }

        void operator*=(const float value)
        {
            x *= value;
            y *= value;
            z *= value;
        }

        Vector3 operator+(const Vector3& b) const { return Vector3(x + b.x, y + b.y, z + b.z); }
        Vector3 operator+(const float value) const { return Vector3(x + value, y + value, z + value); }

        void operator+=(const Vector3& b)
        {
            x += b.x;
            y += b.y;
            z += b.z;
        }

        void operator+=(const float value)
        {
            x += value;
            y += value;
            z += value;
        }

        Vector3 operator-(const Vector3& b) const { return Vector3(x - b.x, y - b.y, z - b.z); }
        Vector3 operator-(const float value) const { return Vector3(x - value, y - value, z - value); }

        void operator-=(const Vector3& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
        }

        Vector3 operator/(const Vector3& rhs) const { return Vector3(x / rhs.x, y / rhs.y, z / rhs.z); }
        Vector3 operator/(const float rhs) const { return Vector3(x / rhs, y / rhs, z / rhs); }

        void operator/=(const Vector3& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
        }

        bool operator==(const Vector3& rhs) const
        {
            return x == rhs.x && y == rhs.y && z == rhs.z;
        }

        bool operator!=(const Vector3& rhs) const
        {
            return !(*this == rhs);
        }

        Vector3 operator -() const { return Vector3(-x, -y, -z); }

        std::string ToString() const;
        inline const float* Data() const { return &x; }

        float x;
        float y;
        float z;

        static const Vector3 Zero;
        static const Vector3 Left;
        static const Vector3 Right;
        static const Vector3 Up;
        static const Vector3 Down;
        static const Vector3 Forward;
        static const Vector3 Backward;
        static const Vector3 One;
        static const Vector3 Inf;
        static const Vector3 NegInf;
	};

	inline Vector3 operator*(float lhs, const Vector3& rhs)
	{
		return rhs * lhs;
	}
}

