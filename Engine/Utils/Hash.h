#pragma once

// ������ ���̵� �ο��ϱ� ���� �ؽ�

namespace PlayGround::Utility::Hash
{
	template <typename T>
	constexpr void HashCombine(uint32_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
}