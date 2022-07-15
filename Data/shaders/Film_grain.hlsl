#include "Common.hlsl"

static const float g_film_grain_intensity = 0.002f;
static const float g_film_grain_speed = 3.0f;
static const float g_film_grain_mean = 0.0f; // What gray level noise should tend to.
static const float g_film_grain_variance = 0.5f; // Controls the contrast/variance of noise.

float gaussian(float z, float u, float o)
{
	return (1.0f / (o * sqrt(2.0f * PI))) * exp(-(((z - u) * (z - u)) / (2.0f * (o * o))));
}

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
	// Out of bounds check
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;

	const float2 uv = (thread_id.xy + 0.5f) / g_resolution_rt;
	float3 color = tex[thread_id.xy].rgb;
	
	// Film grain
	float t = g_time * float(g_film_grain_speed);
	float seed = dot(uv, float2(12.9898f, 78.233f));
	float noise = frac(sin(seed) * 43758.5453f + t);
	noise = gaussian(noise, float(g_film_grain_mean), float(g_film_grain_variance) * float(g_film_grain_variance));
	float film_gran = noise * g_film_grain_intensity;
	
	// ISO noise
	float iso_noise = get_random(frac(uv.x * uv.y * g_time)) * g_camera_iso * 0.000002f;

	// Additive blending
	color.rgb += (film_gran + iso_noise) * 0.5f;
	
	tex_out_rgb[thread_id.xy] = saturate(color);
}