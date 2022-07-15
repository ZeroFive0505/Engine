#include "Common.hlsl"

#if LUMINANCE

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
	// 스레드 바운드 체크
    if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
        return;
        
    float3 color = tex[thread_id.xy].rgb;
    tex_out_rgb[thread_id.xy] = saturate_16(luminance(color) * color);
}

#endif

#if UPSAMPLE_BLEND_MIP

float3 tent_antiflicker_filter(float2 uv, float2 texel_size)
{
    // 텐트 필터
    float4 d  = texel_size.xyxy * float4(-1.0f, -1.0f, 1.0f, 1.0f) * 2.0f;
    float3 s1 = tex.SampleLevel(sampler_bilinear_clamp, uv + d.xy, 0.0f).rgb;
    float3 s2 = tex.SampleLevel(sampler_bilinear_clamp, uv + d.zy, 0.0f).rgb;
    float3 s3 = tex.SampleLevel(sampler_bilinear_clamp, uv + d.xw, 0.0f).rgb;
    float3 s4 = tex.SampleLevel(sampler_bilinear_clamp, uv + d.zw, 0.0f).rgb;
    
    // 평균치
    float s1w = 1.0f / (luminance(s1) + 1.0f);
    float s2w = 1.0f / (luminance(s2) + 1.0f);
    float s3w = 1.0f / (luminance(s3) + 1.0f);
    float s4w = 1.0f / (luminance(s4) + 1.0f);
    float one_div_wsum = 1.0 / (s1w + s2w + s3w + s4w);
    
    return (s1 * s1w + s2 * s2w + s3 * s3w + s4 * s4w) * one_div_wsum;
}

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
    // 스레드 바운드 체크
    if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
        return;

    const float2 uv           = (thread_id.xy + 0.5f) / g_resolution_rt;
    float3 upsampled_color    = tent_antiflicker_filter(uv, g_texel_size * 0.5f);
    tex_out_rgb[thread_id.xy] = saturate_16(tex_out_rgb[thread_id.xy] + upsampled_color * 0.5f);
}

#endif

#if BLEND_FRAME

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
    // 스레드 바운드 체크
    if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
        return;

    float3 color_frame  = tex[thread_id.xy].rgb;
    float3 color_mip    = tex2[thread_id.xy].rgb;
    tex_out_rgb[thread_id.xy] = saturate_16(color_frame + color_mip * g_bloom_intensity);
}

#endif