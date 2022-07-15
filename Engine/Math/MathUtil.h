#pragma once

#include <cmath>
#include <limits>
#include <random>

namespace PlayGround::Math
{
	enum class Intersection
	{
		Outside,
		Inside,
		Intersects
	};
}

namespace PlayGround::Math::Util
{
    constexpr float EPSILON = std::numeric_limits<float>::epsilon();
    constexpr float INFINITY_ = std::numeric_limits<float>::infinity();
    constexpr float PI = 3.14159265359f;
    constexpr float PI_2 = 6.28318530718f;
    constexpr float PI_4 = 12.5663706144f;
    constexpr float PI_DIV_2 = 1.57079632679f;
    constexpr float PI_DIV_4 = 0.78539816339f;
    constexpr float PI_INV = 0.31830988618f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    inline float Cot(float x)
    {
        return cos(x) / sin(x);
    }

    constexpr float DegreesToRadians(const float degree)
    {
        return degree * DEG_TO_RAD;
    }

    constexpr float RadiansToDegrees(const float radians)
    {
        return radians * RAD_TO_DEG;
    }

    template <typename T>
    constexpr T Clamp(T low, T high, T val)
    {
        if (val < low)
            return low;
        else if (val > high)
            return high;
        
        return val;
    }

    template <typename T>
    constexpr T Saturate(T val)
    {
        return Clamp<T>(static_cast<T>(0), static_cast<T>(1), val);
    }

    template <typename T, typename U>
    constexpr T Lerp(T lhs, T rhs, U t)
    {
        return lhs * (static_cast<U>(1) - t) + rhs * t;
    }

    template <typename T>
    constexpr T Abs(T val)
    {
        return val >= static_cast<T>(0) ? val : -val;
    }

    template <typename T>
    constexpr bool Equals(T lhs, T rhs, T error = std::numeric_limits<T>::epsilon())
    {
        return lhs + error >= rhs && lhs - error <= rhs;
    }

    template <typename T>
    constexpr T Max(T a, T b)
    {
        return a > b ? a : b;
    }

    template <typename T>
    constexpr T Max3(T a, T b, T c)
    {
        return Max(a, Max(b, c));
    }

    template <typename T>
    constexpr T Min(T a, T b)
    {
        return a < b ? a : b;
    }

    template <typename T>
    constexpr T Min3(T a, T b, T c)
    {
        return Min(a, Min(b, c));
    }

    template <typename T>
    constexpr T Sqrt(T x)
    {
        return sqrt(x);
    }

    template <typename T>
    constexpr T Floor(T x)
    {
        return floor(x);
    }

    template <typename T>
    constexpr T Ceil(T x)
    {
        return ceil(x);
    }

    template <typename T>
    constexpr T Round(T x)
    {
        return round(x);
    }

    template <typename T>
    constexpr T Tan(T x)
    {
        return tan(x);
    }

    template <typename T>
    constexpr T Cos(T x)
    {
        return cos(x);
    }

    template <typename T>
    constexpr T Sin(T x)
    {
        return sin(x);
    }

    template <typename T>
    constexpr int Sign(T x)
    {
        return (static_cast<T>(0) < x) - (x < static_cast<T>(0));
    }

    template <typename T>
    constexpr T Pow(T x, T y)
    {
        return pow(x, y);
    }

    template <typename T>
    constexpr T Log(T x)
    {
        return log(x);
    }

    template <typename T>
    T Random(T from = static_cast < T>0, T to = static_cast<T>(1))
    {
        std::random_device rd;
        std::mt19937 eng(rd());
        std::uniform_real_distribution<T> dist(from, to);
        return dist(eng);
    }
}