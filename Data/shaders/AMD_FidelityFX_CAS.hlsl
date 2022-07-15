#include "Common.hlsl"

#define A_GPU
#define A_HLSL

#include "ffx_a.h"

// Functions ffx_cas.h wants defined
float3 CasLoad(float2 pos)
{
	return tex[pos].rgb;
}

// Lets you transform input from the load into a linear color space between 0 and 1.
void CasInput(inout float r, inout float g, inout float b)
{

}

#include "ffx_cas.h"

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
	// Out of bounds check
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;

	float4 const0;
	float4 const1;
	CasSetup(const0, const1, g_sharpen_strength, g_resolution_rt.x, g_resolution_rt.y, g_resolution_rt.x, g_resolution_rt.y);

	float3 color = 0.0f;
	CasFilter(color.r, color.g, color.b, thread_id.xy, const0, const1, true);
	
	tex_out_rgb[thread_id.xy] = color;
}