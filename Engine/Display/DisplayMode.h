#pragma once

namespace PlayGround
{
	// ���� ���÷��� ���¸� ��Ÿ���� ����ü
	// ���÷����� ����, ����, ���ļ����� �����Ѵ�.
	struct sDisplayMode
	{
		sDisplayMode() = default;
		sDisplayMode(const uint32_t _width, const uint32_t _height, const uint32_t _numerator, const uint32_t _denominator)
		{
			width = _width;
			height = _height;
			numerator = _numerator;
			denominator = _denominator;
			hz = static_cast<double>(numerator) / static_cast<double>(denominator);
		}

		bool operator==(const sDisplayMode& rhs) const
		{
			return width == rhs.width && height == rhs.height && hz == rhs.hz;
		}

		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t numerator = 0;
		uint32_t denominator = 0;
		double hz = 0;
	};
}