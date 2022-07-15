#pragma once

// 고유한 아이디를 부여하기 위한 해시

namespace PlayGround::Utility::Hash
{
	template <typename T>
	constexpr void HashCombine(uint32_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
}