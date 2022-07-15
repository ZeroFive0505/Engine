#include "Common.hlsl"

Pixel_PosUV mainVS(Vertex_PosUV input)
{
	Pixel_PosUV output;
	
	input.position.w = 1.0f;
	output.position = mul(input.position, g_view_projection_orthographic);
	output.uv = input.uv;

	return output;
}