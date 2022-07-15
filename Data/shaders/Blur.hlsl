#include "Common.hlsl"

static const int g_blur_radius = 5;
static const float g_threshold = 0.1f;

// 가우시간 가중치 계산
float compute_gaussian_weight(int sample_distance)
{
	float sigma2 = g_blur_sigma * g_blur_sigma;
	float g = 1.0f / sqrt(2.0f * 3.14159f * sigma2);
	return (g * exp(-(sample_distance * sample_distance) / (2.0f * sigma2)));
}

// 가우시안 블러
// https://github.com/TheRealMJP/MSAAFilter/blob/master/MSAAFilter/PostProcessing.hlsl#L50
float4 gaussian_blur(const uint2 pos)
{
	const float2 uv = (pos.xy + 0.5f) / g_resolution_in;
	const float2 texel_size = float2(1.0f / g_resolution_in.x, 1.0f / g_resolution_in.y);
	const float2 direction = texel_size * g_blur_direction;
	const bool is_vertical_pass = g_blur_direction.y != 0.0f;

	float weight_sum = 0.0f;
	float4 color = 0;
    [unroll]
	for (int i = -g_blur_radius; i < g_blur_radius; i++)
	{
		float2 sample_uv = uv + (i * direction);

        // During the vertical pass, the input texture is seconday scratch texture which belongs to the blur pass.
        // It's at least as big as the original input texture (to be blurred), so we have to adapt the smaple uv.
		sample_uv = lerp(sample_uv, (trunc(sample_uv * g_resolution_in) + 0.5f) / g_resolution_rt, is_vertical_pass);
        
		float weight = compute_gaussian_weight(i);
		color += tex.SampleLevel(sampler_bilinear_clamp, sample_uv, 0) * weight;
		weight_sum += weight;
	}

	return color / weight_sum;
}

// 깊이 인식 가우시안 블러
float4 depth_aware_gaussian_blur(const uint2 pos)
{
	const float2 uv = (pos.xy + 0.5f) / g_resolution_in;
	const float2 texel_size = float2(1.0f / g_resolution_in.x, 1.0f / g_resolution_in.y);
	// 방향 계산
	const float2 direction = texel_size * g_blur_direction;
	// 수직 패스인지 확인
	const bool is_vertical_pass = g_blur_direction.y != 0.0f;
    
	// 가운데 깊이, 노말 값
	const float center_depth = get_linear_depth(uv);
	const float3 center_normal = get_normal(uv);

	float weight_sum = 0.0f;
	float4 color = 0.0f;
    [unroll]
	for (int i = -g_blur_radius; i < g_blur_radius; i++)
	{
		// 블러 방향
		float2 sample_uv = uv + (i * direction);
		// 깊이, 노말
		float sample_depth = get_linear_depth(sample_uv);
		float3 sample_normal = get_normal(sample_uv);
        
        // 깊이 인식
		float awareness_depth = saturate(g_threshold - abs(center_depth - sample_depth));
		// FLT_MIN을 더해 0이되는 것을 방지
		float awareness_normal = saturate(dot(center_normal, sample_normal)) + FLT_MIN;
		// 곱
		float awareness = awareness_normal * awareness_depth;

        // During the vertical pass, the input texture is seconday scratch texture which belongs to the blur pass.
        // It's at least as big as the original input texture (to be blurred), so we have to adapt the smaple uv.
		sample_uv = lerp(sample_uv, (trunc(sample_uv * g_resolution_in) + 0.5f) / g_resolution_rt, is_vertical_pass);

		float weight = compute_gaussian_weight(i) * awareness;
		color += tex.SampleLevel(sampler_bilinear_clamp, sample_uv, 0) * weight;
		weight_sum += weight;
	}

	return color / weight_sum;
}

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
    // Out of bounds check
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;

	float4 color = 0.0f;

#if PASS_BLUR_GAUSSIAN
    color = gaussian_blur(thread_id.xy);
#endif

#if PASS_BLUR_BILATERAL_GAUSSIAN
    color = depth_aware_gaussian_blur(thread_id.xy);
#endif
    
	tex_out_rgba[thread_id.xy] = color;
}
