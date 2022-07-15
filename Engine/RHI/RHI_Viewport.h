#pragma once

#include "../Core/EngineObject.h"

namespace PlayGround
{
	// RHI �� ��Ʈ
	class RHI_Viewport
	{
	public:
		RHI_Viewport(const float _x = 0.0f, const float _y = 0.0f, const float _width = 0.0f, const float _height = 0.0f,
			const float _depth_min = 0.0f, const float _depth_max = 1.0f)
		{
			x = _x;
			y = _y;
			width = _width;
			height = _height;
			depth_min = _depth_min;
			depth_max = _depth_max;
		}

		RHI_Viewport(const RHI_Viewport& viewport)
		{
			x = viewport.x;
			y = viewport.y;
			width = viewport.width;
			height = viewport.height;
			depth_min = viewport.depth_min;
			depth_max = viewport.depth_max;
		}

		~RHI_Viewport() = default;

		bool operator==(const RHI_Viewport& rhs) const
		{
			return
				x == rhs.x && y == rhs.y &&
				width == rhs.width && height == rhs.height &&
				depth_min == rhs.depth_min && depth_max == rhs.depth_max;
		}

		bool operator!=(const RHI_Viewport& rhs) const
		{
			return !(*this == rhs);
		}

		bool IsDefined() const
		{
			return 
				x != 0.0f ||
				y != 0.0f ||
				width != 0.0f ||
				height != 0.0f ||
				depth_min != 0.0f ||
				depth_max != 0.0f;
		}

		inline float GetAspectRatio() const
		{
			return width / height;
		}

		float x = 0.0f;
		float y = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
		float depth_min = 0.0f;
		float depth_max = 0.0f;

		static const RHI_Viewport Undefined;
	};
}

