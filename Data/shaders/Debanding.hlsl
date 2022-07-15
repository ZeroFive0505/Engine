#include "Common.hlsl"

static const float g_debanding_offset = 0.5f / 255.0f;

// http://alex.vlachos.com/graphics/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf
inline float3 dither(uint2 screen_pos)
{
	float3 dither = dot(float2(171.0f, 231.0f), float2(screen_pos));
	dither = frac(dither / float3(103.0f, 71.0f, 97.0f));
	dither /= 255.0f;
    
	return dither;
}

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
	// Out of bounds check
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;
	
	float3 color = tex[thread_id.xy].rgb;
	float rnd = dither(thread_id.xy * g_resolution_rt.xy).x;

	tex_out_rgb[thread_id.xy] = color + lerp(-g_debanding_offset, g_debanding_offset, rnd);
}