#include "Common.hlsl"

static const float g_chromatic_aberration_intensity = 100.0f;

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
	// ¹Ù¿îµå Ã¼Å©
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;

	const float2 uv = (thread_id.xy + 0.5f) / g_resolution_rt;
	float camera_error = 1.0f / g_camera_aperture;
	float intensity = camera_error * g_chromatic_aberration_intensity;
	float2 shift = float2(intensity, -intensity);

	// ·»Áî ÀÌÆåÆ®
	shift.x *= abs(uv.x * 2.0f - 1.0f);
	shift.y *= abs(uv.y * 2.0f - 1.0f);
	
	// Ä®¶ó »ùÇÃ¸µ
	float3 color = 0.0f;
	color.r = tex.SampleLevel(sampler_bilinear_clamp, uv + (g_texel_size * shift), 0).r;
	color.g = tex[thread_id.xy].g;
	color.b = tex.SampleLevel(sampler_bilinear_clamp, uv - (g_texel_size * shift), 0).b;

	tex_out_rgb[thread_id.xy] = color;
}