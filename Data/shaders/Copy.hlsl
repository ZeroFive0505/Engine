#include "Common.hlsl"

#if COMPUTE

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
    // Out of bounds check
    if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
        return;

#if BILINEAR
    const float2 uv = (thread_id.xy + 0.5f) / g_resolution_rt;
    tex_out_rgb[thread_id.xy] = tex.SampleLevel(sampler_bilinear_clamp, uv, 0).rgb;
#else
    tex_out_rgb[thread_id.xy] = tex[thread_id.xy].rgb;
#endif
}

#elif PIXEL

float4 mainPS(Pixel_PosUV input) : SV_TARGET
{
#if BILINEAR
    return tex.Sample(sampler_bilinear_clamp, input.uv);
#else
    return tex.Sample(sampler_point_clamp, input.uv);
#endif
}

#endif
