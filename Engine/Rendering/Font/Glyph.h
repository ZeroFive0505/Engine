#pragma once

namespace PlayGround
{
	struct Glyph
	{
		// 오프셋
		int32_t offset_x = 0;
		int32_t offset_y = 0;
		// 높이, 넓이
		uint32_t width = 0;
		uint32_t height = 0;
		// 자간
		uint32_t horizontal_advance = 0;
		// UV 좌표
		float uv_x_left = 0.0f;
		float uv_x_right = 0.0f;
		float uv_y_top = 0.0f;
		float uv_y_bottom = 0.0f;
	};
}