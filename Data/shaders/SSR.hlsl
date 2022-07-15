#include "Common.hlsl"

static const float g_ssr_max_distance = 100.0f;
static const uint g_ssr_max_steps = 64;
static const uint g_ssr_binary_search_steps = 24;
static const float g_ssr_thickness = 0.0001f;

float compute_alpha(uint2 screen_pos, float2 hit_uv, float v_dot_r)
{
	float alpha = 1.0f;
	
	alpha *= screen_fade(hit_uv);

	// If the UV is invalid fade completely
	alpha *= all(hit_uv);

	return alpha;
}

float get_depth_from_ray(float2 ray_pos, float2 ray_start, float ray_length, float z_start, float z_end)
{
	float alpha = length(ray_pos - ray_start) / ray_length;
	return (z_start * z_end) / lerp(z_end, z_start, alpha);
}

bool intersect_depth_buffer(float2 ray_pos, float2 ray_start, float ray_length, float z_start, float z_end, out float depth_delta)
{
	float depth_ray = get_depth_from_ray(ray_pos, ray_start, ray_length, z_start, z_end);
	float depth_real = get_linear_depth(ray_pos);
	depth_delta = depth_ray - depth_real;
	
	return depth_delta >= 0.0f;
}

bool lines_intersect(float2 a1, float2 a2, float2 b1, float2 b2, out float2 intersection)
{
	intersection = 0.0f;
	
	float2 b = a2 - a1;
	float2 d = b2 - b1;
	float bDotDPerp = b.x * d.y - b.y * d.x;
	
	// if b dot d == 0, it means the lines are parallel so have infinite intersection points
	if (bDotDPerp == 0.0f)
		return false;

	float2 c = b1 - a1;
	float t = (c.x * d.y - c.y * d.x) / bDotDPerp;

	if (t < 0.0f || t > 1.0f)
		return false;

	float u = (c.x * b.y - c.y * b.x) / bDotDPerp;
	
	if (u < 0.0f || u > 1.0f)
		return false;
	
	intersection = a1 + t * b;

	return true;
}

float2 clip_uv_ray_end_to_bounds(const float2 ray_start, const float2 ray_end)
{
	float2 top_left = float2(0.0f, 0.0f);
	float2 top_right = float2(1.0f, 0.0f);
	float2 bottom_right = float2(1.0f, 1.0f);
	float2 bottom_left = float2(0.0f, 1.0f);

	float2 intersection = 0.0f;
	
	// Top
	if (lines_intersect(ray_start, ray_end, top_left, top_right, intersection))
		return intersection;

    // Right
	if (lines_intersect(ray_start, ray_end, top_right, bottom_right, intersection))
		return intersection;

    // Bottom
	if (lines_intersect(ray_start, ray_end, bottom_right, bottom_left, intersection))
		return intersection;

    // Left
	if (lines_intersect(ray_start, ray_end, bottom_left, top_left, intersection))
		return intersection;
        
	return false;
}

float2 trace_ray(uint2 screen_pos, float3 ray_start_vs, float3 ray_dir_vs)
{
	// Compute ray end and start depth
	float3 ray_end_vs = ray_start_vs + ray_dir_vs * g_ssr_max_distance;
	float depth_end = ray_end_vs.z;
	float depth_start = ray_start_vs.z;
	
	// Compute ray start and end (in UV space)
	float2 ray_start = view_to_uv(ray_start_vs);
	float2 ray_end = view_to_uv(ray_end_vs);

    // Clip ray end to uv bounds
    //float2 ray_end_clipped = clip_uv_ray_end_to_bounds(ray_start, ray_end); 
    //depth_end              = get_depth_from_ray(ray_end_clipped, ray_start, length(ray_end_clipped - ray_start), depth_start, depth_end);
    //ray_end                = ray_end_clipped;
	
	// Compute ray step
	float2 ray_start_to_end = ray_end - ray_start;
	float ray_length = length(ray_start_to_end);
	float2 ray_step = (ray_start_to_end) / (float) g_ssr_max_steps;
	float2 ray_pos = ray_start;
	
	// Adjust position with some temporal noise (TAA will do some magic later)
	float offset = get_noise_interleaved_gradient(screen_pos);
	ray_pos += ray_step * offset;
	
	// Ray-march
	for (uint i = 0; i < g_ssr_max_steps; i++)
	{
		// Ealry exit if the ray is out of screen
		if (!is_valid_uv(ray_pos))
			return 0.0f;

		// intersect depth buffer
		float depth_delta = 0.0f;
		if(intersect_depth_buffer(ray_pos, ray_start, ray_length, depth_start, depth_end, depth_delta))
		{
			// Binary search
			float depth_delta_previous_sign = -1.0f;
			
			for (uint j = 0; j < g_ssr_binary_search_steps; j++)
			{
				// Depth test
				if (abs(depth_delta) <= g_ssr_thickness)
					return ray_pos;
					
				// Half direction and flip (if necessery)
				if (sign(depth_delta) != depth_delta_previous_sign)
				{
					ray_step *= -0.5f;
					depth_delta_previous_sign = sign(depth_delta);
				}
				
				// Step ray
				ray_pos += ray_step;
				
				// Intersect depth buffer
				intersect_depth_buffer(ray_pos, ray_start, ray_length, depth_start, depth_end, depth_delta);
			}
			
			return 0.0f;
		}
		
		// Step ray
		ray_pos += ray_step;
	}
	
	return 0.0f;
}

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
	// Out of bounds check
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;

	// Construct surface
	Surface surface;
	surface.Build(thread_id.xy, false, false, false);
	
	float alpha = 0.0f;
	float3 color = 0.0f;
	
	if (surface.is_sky())
	{
		tex_out_rgba[thread_id.xy] = float4(color, alpha);
		return;
	}
	
	// Skip pixels which are fully rough
	float2 hit_uv = -1.0f;
	
	if (surface.roughness < 0.8f) // save some performance by skipping pixels that won't really show reflections
	{
		// Compute reflection direction in view space
		float3 normal = get_normal_view_space(thread_id.xy);	
		float3 position = get_position_view_space(thread_id.xy);
		float3 camera_to_pixel = normalize(position);
		float3 reflection = normalize(reflect(camera_to_pixel, normal));
		float v_dot_r = dot(-camera_to_pixel, reflection);

		// Dont't trace rays which facing the camera
		if (v_dot_r < 0.0f)
		{
			hit_uv = trace_ray(thread_id.xy, position, reflection);
			alpha = compute_alpha(thread_id.xy, hit_uv, v_dot_r);
		}
	}
	
	// Sample scene color
	hit_uv -= tex_velocity.SampleLevel(sampler_bilinear_clamp, hit_uv, 0).xy; // reproject
	bool valid_uv = hit_uv.x != -1.0f;
	bool valid_alpha = alpha != 0.0f;
	color = (valid_uv && valid_alpha) ? tex.SampleLevel(sampler_bilinear_clamp, hit_uv, 0).rgb : 0.0f;

	tex_out_rgba[thread_id.xy] = float4(color, alpha);
}