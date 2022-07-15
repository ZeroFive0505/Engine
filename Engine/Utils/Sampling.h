#pragma once

#include "../Math/Vector2.h"

// 노이즈 샘플링 함수

namespace PlayGround::Utility::Sampling
{
	inline float Halton(uint64_t index, uint64_t base)
	{
		float f = 1.0f;
		float r = 0.0f;

		while (index > 0)
		{
			f = f / static_cast<float>(base);
			r = r + f * (index % base);
			index = index / base;
		}

		return r;
	}

	inline Math::Vector2 Halton2D(uint64_t index, uint64_t baseA, uint64_t baseB)
	{
		return Math::Vector2(Halton(index, baseA), Halton(index, baseB));
	}
}