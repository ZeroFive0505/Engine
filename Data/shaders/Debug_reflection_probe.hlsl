#include "Common.hlsl"

Pixel_PosNor mainVS(Vertex_PosUVNorTan input)
{
	Pixel_PosNor output;
	
	float4x4 wvp = mul(g_transform, g_view_projection_unjittered);
	
	input.position.w = 1.0f;
	output.position = mul(input.position, wvp);
	output.normal = normalize(mul(input.normal, (float3x3) g_transform)).xyz;

	return output;
}

float4 mainPS(Pixel_PosNor input) : SV_TARGET
{
	return float4(tex_reflection_probe.SampleLevel(sampler_bilinear_clamp, reflect(g_camera_direction, input.normal), 0.0f).rgb, 1.0f);
}