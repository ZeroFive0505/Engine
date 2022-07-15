#include "Common.hlsl"

static const uint g_ao_directions = 2;
static const uint g_ao_steps = 2;
static const float g_ao_radius = 2.0f;
static const float g_ao_occlusion_bias = 0.0f;

static const float ao_samples = (float) (g_ao_directions * g_ao_steps);
static const float ao_samples_rcp = 1.0f / ao_samples;
static const float ao_radius2 = g_ao_radius * g_ao_radius;
static const float ao_negInvRadius2 = -1.0f / ao_radius2;

float compute_falloff(float distance_squared)
{
	return saturate(distance_squared * ao_negInvRadius2 + 1.0f);
}

float compute_visibility(float3 original_normal, float3 original_to_sample)
{
	float distance_squared = dot(original_to_sample, original_to_sample);
	float n_dot_v = dot(original_normal, original_to_sample) * rsqrt(distance_squared);
	float falloff = compute_falloff(distance_squared);

	return saturate(n_dot_v - g_ao_occlusion_bias) * falloff;
}

// Screen space directional and temporal ground truth ambient occlusion with global illumination and bent normals.
void compute_uber_ssao(uint2 pos, inout float3 bent_normal, inout float occlusion, inout float3 diffuse_bounce)
{
	const float2 origin_uv = (pos + 0.5f) / g_resolution_rt;
	const float3 origin_position = get_position_view_space(pos);
	const float3 origin_normal = get_normal_view_space(pos);

	// Compute step in pixels
	const float pixel_offset = max((g_ao_radius * g_resolution_rt.x * 0.5f) / origin_position.z, (float) g_ao_steps);
	const float step_offset = pixel_offset / float(g_ao_steps + 1.0f); // divide by steps + 1 so that the farthest samples are not fully attenuated
	
	// Compute rotation step
	const float step_direction = PI2 / (float) g_ao_directions;
	
	// Offsets (noise over space and time)
	const float noise_gradient_temporal = get_noise_interleaved_gradient(pos);
	const float offset_spatial = get_offset_non_temporal(pos);
	const float offset_temporal = get_offset();
	const float offset_rotation_temporal = get_direction();
	const float ray_offset = frac(offset_spatial + offset_temporal) + (get_random(origin_uv) * 2.0f - 1.0f) * 0.25f;
	
	// Compute light / occlusiton and bend normal
	for (uint direction_index = 0; direction_index < g_ao_directions; direction_index++)
	{
		float rotation_angle = (direction_index + noise_gradient_temporal + offset_rotation_temporal) * step_direction;
		float2 rotation_direction = float2(cos(rotation_angle), sin(rotation_angle)) * g_texel_size;

		for (uint step_index = 0; step_index < g_ao_steps; step_index++)
		{
			float2 uv_offset = round(max(step_offset * (step_index + ray_offset), 1 + step_index)) * rotation_direction;
			uint2 sample_pos = (origin_uv + uv_offset) * g_resolution_rt;
			float3 sample_position = get_position_view_space(sample_pos);
			float3 origin_to_sample = sample_position - origin_position;
			float visibillity = compute_visibility(origin_normal, origin_to_sample) * screen_fade(origin_uv);

			occlusion += visibillity;
			bent_normal += normalize(origin_to_sample) * visibillity;

			
			// Light
			if (is_ssao_gi_enabled())
			{
				diffuse_bounce += tex_light_diffuse[sample_pos].rgb * tex_albedo[sample_pos].rgb * visibillity;
			}
		}
	}
	
	bent_normal *= ao_samples_rcp;
	occlusion = 1.0f - saturate(occlusion * ao_samples_rcp);
	diffuse_bounce = saturate(diffuse_bounce);
}

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
	// Out of bounds check
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;

	float3 bent_normal = 0.0f;
	float occlusion = 0.0f;
	float3 diffuse_bounce = 0.0f;
	
	compute_uber_ssao(thread_id.xy, bent_normal, occlusion, diffuse_bounce);

	tex_out_rgba[thread_id.xy] = float4(bent_normal, occlusion);
	tex_out_rgba2[thread_id.xy] = float4(diffuse_bounce, 1.0f);
}