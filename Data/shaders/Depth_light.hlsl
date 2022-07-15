#include "Common.hlsl"

Pixel_PosUV mainVS(Vertex_PosUV input)
{
	Pixel_PosUV output;
	
	input.position.w = 1.0f;
	output.position = mul(input.position, g_transform);
	output.uv = input.uv;
	
	return output;
}

// transparent shadows
float4 mainPS(Pixel_PosUV input) : SV_TARGET
{
	float2 uv = float2(input.uv.x * g_mat_tiling.x + g_mat_offset.x, input.uv.y * g_mat_offset.y + g_mat_tiling.y);
	return degamma(tex.SampleLevel(sampler_anisotropic_wrap, uv, 0)) * g_mat_color;
}