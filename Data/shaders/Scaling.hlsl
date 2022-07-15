#include "Common.hlsl"

float4 Box_Filter(float2 uv, Texture2D tex, float2 texel_size)
{
	float4 offset = texel_size.xyxy * float4(-1.0f, -1.0f, 1.0f, 1.0f);
	
	float4 samples =
    tex.SampleLevel(sampler_bilinear_clamp, uv + offset.xy, 0) +
    tex.SampleLevel(sampler_bilinear_clamp, uv + offset.zy, 0) +
    tex.SampleLevel(sampler_bilinear_clamp, uv + offset.xw, 0) +
    tex.SampleLevel(sampler_bilinear_clamp, uv + offset.zw, 0);

	return samples / 4.0f;
}