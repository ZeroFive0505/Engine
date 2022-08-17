#pragma once

#include "Vector3.h"

namespace PlayGround::Math
{
	class Quaternion
	{
    public:
        Quaternion()
        {
            x = 0;
            y = 0;
            z = 0;
            w = 1;
        }

        Quaternion(float x, float y, float z, float w)
        {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }
        ~Quaternion() = default;

        // �־��� ���� ������ ������� ������.
        // ������ ����
        static inline Quaternion FromAngleAxis(float angle, const Vector3& axis)
        {
            const float half = angle * 0.5f;
            const float sin = sinf(half);
            const float cos = cosf(half);

            return Quaternion(axis.x * sin, axis.y * sin, axis.z * sin, cos);
        }

        void FromAxes(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis);

        // �־��� yaw, pitch, roll�� ������ ������� ������.
        static inline Quaternion FromYawPitchRoll(float yaw, float pitch, float roll)
        {
            const float halfRoll = roll * 0.5f;
            const float halfPitch = pitch * 0.5f;
            const float halfYaw = yaw * 0.5f;

            const float sinRoll = sin(halfRoll);
            const float cosRoll = cos(halfRoll);
            const float sinPitch = sin(halfPitch);
            const float cosPitch = cos(halfPitch);
            const float sinYaw = sin(halfYaw);
            const float cosYaw = cos(halfYaw);

            return Quaternion(
                cosYaw * sinPitch * cosRoll + sinYaw * cosPitch * sinRoll,
                sinYaw * cosPitch * cosRoll - cosYaw * sinPitch * sinRoll,
                cosYaw * cosPitch * sinRoll - sinYaw * sinPitch * cosRoll,
                cosYaw * cosPitch * cosRoll + sinYaw * sinPitch * sinRoll
            );
        }

        static inline Quaternion FromToRotation(const Vector3& start, const Vector3& end)
        {
            const Vector3 normStart = start.Normalized();
            const Vector3 normEnd = end.Normalized();
            const float d = normStart.Dot(normEnd);

            if (d > -1.0f + Util::EPSILON)
            {
                const Vector3 c = normStart.Cross(normEnd);
                const float s = sqrtf((1.0f + d) * 2.0f);
                const float invS = 1.0f / s;

                return Quaternion(
                    c.x * invS,
                    c.y * invS,
                    c.z * invS,
                    0.5f * s);
            }
            else
            {
                Vector3 axis = Vector3::Right.Cross(normStart);
                if (axis.Length() < Util::EPSILON)
                {
                    axis = Vector3::Up.Cross(normStart);
                }

                return FromAngleAxis(180.0f * Util::DEG_TO_RAD, axis);
            }
        }

        static inline Quaternion FromLookRotation(const Vector3& direction, const Vector3& up_direction = Vector3::Up)
        {
            Quaternion result;
            const Vector3 forward = direction.Normalized();

            Vector3 v = forward.Cross(up_direction);
            if (v.LengthSquared() >= Util::EPSILON)
            {
                v.Normalize();
                const Vector3 up = v.Cross(forward);
                const Vector3 right = up.Cross(forward);
                result.FromAxes(right, up, forward);
            }
            else
            {
                result = Quaternion::FromToRotation(Vector3::Forward, forward);
            }

            return result;
        }

        static inline Quaternion FromToRotation(const Quaternion& start, const Quaternion& end) { return start.Inverse() * end; }

        static inline Quaternion Lerp(const Quaternion& a, const Quaternion& b, const float t)
        {
            Quaternion quaternion;

            if (Dot(a, b) >= 0)
            {
                quaternion = a * (1 - t) + b * t;
            }
            else
            {
                quaternion = a * (1 - t) - b * t;
            }

            return quaternion.Normalized();
        }

        static inline Quaternion Multiply(const Quaternion& Qa, const Quaternion& Qb)
        {
            const float x = Qa.x;
            const float y = Qa.y;
            const float z = Qa.z;
            const float w = Qa.w;
            const float num4 = Qb.x;
            const float num3 = Qb.y;
            const float num2 = Qb.z;
            const float num = Qb.w;
            const float num12 = (y * num2) - (z * num3);
            const float num11 = (z * num4) - (x * num2);
            const float num10 = (x * num3) - (y * num4);
            const float num9 = ((x * num4) + (y * num3)) + (z * num2);

            return Quaternion(
                ((x * num) + (num4 * w)) + num12,
                ((y * num) + (num3 * w)) + num11,
                ((z * num) + (num2 * w)) + num10,
                (w * num) - num9
            );
        }

        inline auto Conjugate() const { return Quaternion(-x, -y, -z, w); }
        inline float LengthSquared() const { return (x * x) + (y * y) + (z * z) + (w * w); }

        void Normalize()
        {
            const float length_squared = LengthSquared();
            if (!Util::Equals(length_squared, 1.0f) && length_squared > 0.0f)
            {
                const auto length_inverted = 1.0f / Util::Sqrt(length_squared);
                x *= length_inverted;
                y *= length_inverted;
                z *= length_inverted;
                w *= length_inverted;
            }
        }

        Quaternion Normalized() const
        {
            const float length_squared = LengthSquared();
            if (!Util::Equals(length_squared, 1.0f) && length_squared > 0.0f)
            {
                const auto length_inverted = 1.0f / Util::Sqrt(length_squared);
                return (*this) * length_inverted;
            }
            else
            {
                return *this;
            }
        }

        Quaternion Inverse() const
        {
            const float length_squared = LengthSquared();
            if (length_squared == 1.0f)
            {
                return Conjugate();
            }
            else if (length_squared >= Util::EPSILON)
            {
                return Conjugate() * (1.0f / length_squared);
            }
            else
            {
                return Identity;
            }
        }

        Vector3 ToEulerAngles() const
        {
            // http://www.geometrictools.com/Documentation/EulerAngles.pdf
            const float check = 2.0f * (-y * z + w * x);

            if (check < -0.995f)
            {
                return Vector3
                (
                    -90.0f,
                    0.0f,
                    -atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) * Util::RAD_TO_DEG
                );
            }

            if (check > 0.995f)
            {
                return Vector3
                (
                    90.0f,
                    0.0f,
                    atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) * Util::RAD_TO_DEG
                );
            }

            return Vector3
            (
                asinf(check) * Util::RAD_TO_DEG,
                atan2f(2.0f * (x * z + w * y), 1.0f - 2.0f * (x * x + y * y)) * Util::RAD_TO_DEG,
                atan2f(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z)) * Util::RAD_TO_DEG
            );
        }

        static inline auto FromEulerAngles(const Vector3& rotation) { return FromYawPitchRoll(rotation.y * Util::DEG_TO_RAD, rotation.x * Util::DEG_TO_RAD, rotation.z * Util::DEG_TO_RAD); }
        static inline auto FromEulerAngles(float rotationX, float rotationY, float rotationZ) { return FromYawPitchRoll(rotationY * Util::DEG_TO_RAD, rotationX * Util::DEG_TO_RAD, rotationZ * Util::DEG_TO_RAD); }

        inline float Yaw() const { return ToEulerAngles().y; }
        inline float Pitch() const { return ToEulerAngles().x; }
        inline float Roll() const { return ToEulerAngles().z; }

        inline float Dot(const Quaternion& rhs)                            const { return w * rhs.w + x * rhs.x + y * rhs.y + z * rhs.z; }
        static inline float Dot(const Quaternion& a, const Quaternion& b) { return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z; }

        inline Quaternion lerp(const Quaternion& rhs, float t) { return ((*this) + ((rhs - (*this)) * t)).Normalized(); }

        Quaternion& operator=(const Quaternion& rhs) = default;

        inline Quaternion operator+(const Quaternion& rhs) const { return Quaternion(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }

        inline Quaternion operator-(const Quaternion& rhs) const { return Quaternion(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }

        inline Quaternion operator-() const { return Quaternion(-x, -y, -z, -w); }

        inline Quaternion operator*(const Quaternion& rhs) const { return Multiply(*this, rhs); }

        void operator*=(const Quaternion& rhs) { *this = Multiply(*this, rhs); }

        Vector3 operator*(const Vector3& rhs) const
        {
            const Vector3 qVec(x, y, z);
            const Vector3 cross1(qVec.Cross(rhs));
            const Vector3 cross2(qVec.Cross(cross1));

            return rhs + 2.0f * (cross1 * w + cross2);
        }

        Quaternion& operator*=(float rhs)
        {
            x *= rhs;
            y *= rhs;
            z *= rhs;
            w *= rhs;

            return *this;
        }

        Quaternion operator*(float rhs) const { return Quaternion(x * rhs, y * rhs, z * rhs, w * rhs); }

        bool operator==(const Quaternion& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
        bool operator!=(const Quaternion& rhs) const { return !(*this == rhs); }
        bool Equals(const Quaternion& rhs)     const { return Util::Equals(x, rhs.x) && Util::Equals(y, rhs.y) && Util::Equals(z, rhs.z) && Util::Equals(w, rhs.w); }

        std::string ToString() const;
        float x, y, z, w;
        static const Quaternion Identity;
	};

	inline Vector3 operator*(const Vector3& lhs, const Quaternion& rhs)
	{
		return rhs * lhs;
	}

	inline Quaternion operator*(float lhs, const Quaternion& rhs)
	{
		return rhs * lhs;
	}
}

