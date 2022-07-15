#include "Common.hlsl"

Pixel_PosUV mainVS(Vertex_PosUV input)
{
	Pixel_PosUV output;
	
	input.position.w = 1.0f;
	output.position = mul(input.position, g_view_projection_orthographic);
	output.uv = input.uv;
	
	return output;
}

float4 mainPS(Pixel_PosUV input) : SV_TARGET
{
	float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
	
	// Sample text from texture atals
	color.r = tex_font_atlas.Sample(sampler_bilinear_clamp, input.uv).r;
	color.g = color.r;
	color.b = color.r;
	color.a = color.r;
	
	// Color it
	color *= g_mat_color;
	
	return color;
}