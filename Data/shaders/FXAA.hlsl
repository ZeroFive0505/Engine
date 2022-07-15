#include "Common.hlsl"
#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 39
#define FXAA_GREEN_AS_LUMA 1
#include "Fxaa3_11.h"

static const float g_fxaa_subPix = 0.75f; // The amount of sub-pixel aliasing removal. This can effect sharpness.
static const float g_fxaa_edgeThreshold = 0.166f; // The minimum amount of local contrast required to apply algorithm.
static const float g_fxaa_edgeThresholdMin = 0.0833f; // Trims the algorithm from processing darks

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
	// Out of bounds check
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;

	const float2 uv = (thread_id.xy + 0.5f) / g_resolution_rt;

	FxaaTex fxaa_tex = { sampler_bilinear_clamp, tex };
	float2 fxaaQualityRcpFrame = g_texel_size;
	
	float3 color = FxaaPixelShader
	(
	     uv, 0, fxaa_tex, fxaa_tex, fxaa_tex,
        fxaaQualityRcpFrame, 0, 0, 0,
        g_fxaa_subPix,
        g_fxaa_edgeThreshold,
        g_fxaa_edgeThresholdMin,
        0, 0, 0, 0
	).rgb;

	tex_out_rgb[thread_id.xy] = color;
}