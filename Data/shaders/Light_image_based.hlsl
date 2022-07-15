#include "BRDF.hlsl"

static const float g_ssr_fallback_threshold_roughness = 0.7f; // value above which blending with the environment texture, is forced.

// From Sebastien Lagarde Moving Frostbite to PBR page 69
float3 get_dominant_specular_direction(float3 normal, float3 reflection, float roughness)
{
	const float smoothness = 1.0f - roughness;
	const float alpha = smoothness * (sqrt(smoothness) + roughness);
	
	return lerp(normal, reflection, alpha);
}

float3 sample_environment(float2 uv, float mip_level)
{
	// We are currently using a spherical environment map which has a smallest mip size of 2x1.
    // So we have to do a bit of blending otherwise we'll get a visible seem in the middle.
	
	if (mip_level >= ENVIRONMENT_MAX_MIP)
	{
		uv = float2(0.5f, 0.0f);
	}
	
	return tex_environment.SampleLevel(sampler_trilinear_clamp, uv, mip_level).rgb;
}

float3 get_parallax_corrected_reflection(Surface surface, float3 position_probe, float3 box_min, float3 box_max)
{
	float3 camera_to_pixel = surface.position - g_camera_position;
	float3 reflection = reflect(camera_to_pixel, surface.normal);
	
	// Find the ray intersection with box plane
	float3 firstPlaneIntersect = (box_max - surface.position) / reflection;
	float3 secondPlaneIntersect = (box_min - surface.position) / reflection;

	// Get the furthest of these intersections along the ray
	float3 furthest_plane = max(firstPlaneIntersect, secondPlaneIntersect);

	// Find the closest far intersection
	float distance = min3(furthest_plane);

	// Get the intersection position
	float3 position_intersection = surface.position + reflection * distance;
	
	// Get correted reflection
	reflection = position_intersection - position_probe;
	
	return reflection;
}

bool is_inside_box(in float3 p, in float3 min, in float3 max)
{
	return (p.x < min.x || p.x > max.x || p.y < min.y || p.y > max.y || p.z < min.z || p.z > max.z) ? false : true;
}

float4 mainPS(Pixel_PosUV input) : SV_TARGET
{
	const uint2 pos = input.uv * g_resolution_rt;
	
	// Construct surface
	Surface surface;
	bool use_ssao = is_opaque_pass(); // we don't do ssao for transparents.
	surface.Build(pos, true, use_ssao, false);

	// If this is a transparent pass, ignore all opaque pixels, and vice versa.
	bool early_exit_1 = is_opaque_pass() && surface.is_transparent();
	bool early_exit_2 = is_transparent_pass() && surface.is_opaque();
	bool early_exit_3 = surface.is_sky(); // we don't want to do IBL on the sky itself.
	if (early_exit_1 || early_exit_2 || early_exit_3)
		discard;
		
	// Just a hack to tone down IBL since it comes from a static texture
	float3 light_ambient = saturate(g_directional_light_intensity / 128000.0f) * surface.occlusion;
	
	// Compute specular energy
	const float n_dot_v = saturate(dot(-surface.camera_to_pixel, surface.normal));
	const float3 F = F_Schlick_Roughness(surface.F0, n_dot_v, surface.roughness);
	const float2 envBRDF = tex_lut_ibl.SampleLevel(sampler_bilinear_clamp, float2(n_dot_v, surface.roughness), 0.0f).xy;
	const float3 specular_energy = F * envBRDF.x + envBRDF.y;
	
	// IBL - Diffuse
	float3 diffuse_energy = compute_diffuse_energy(specular_energy, surface.metallic); // Used to town down diffuse such as that only non metals have it
	float3 ibl_diffuse = sample_environment(direction_sphere_uv(surface.normal), ENVIRONMENT_MAX_MIP) * surface.albedo.rgb * light_ambient * diffuse_energy;
	ibl_diffuse *= surface.alpha; // Fade out for transparents
	
	// IBL - Specular
	const float3 reflection = reflect(surface.camera_to_pixel, surface.normal);
	float3 dominant_specular_direction = get_dominant_specular_direction(surface.normal, reflection, surface.roughness);
	float mip_level = lerp(0, ENVIRONMENT_MAX_MIP, surface.roughness);
	float3 ibl_specular_environment = sample_environment(direction_sphere_uv(dominant_specular_direction), mip_level) * light_ambient;
	
	// Get ssr color
	mip_level = lerp(0, g_ssr_mip_count, surface.roughness);
	const float4 ssr_sample = (is_ssr_enabled() && !g_is_transparent_pass) ? tex_ssr.SampleLevel(sampler_trilinear_clamp, surface.uv, mip_level) : 0.0f;
	const float3 color_ssr = ssr_sample.rgb;
	float ssr_alpha = ssr_sample.a;
	
	// Remap alpha above a certain roughness threshold in order to hide blocky reflections (from very small mips)
	if (surface.roughness > g_ssr_fallback_threshold_roughness)
	{
		ssr_alpha = lerp(ssr_alpha, 0.0f, (surface.roughness - g_ssr_fallback_threshold_roughness) / (1.0f - g_ssr_fallback_threshold_roughness));
	}

    // Sample reflection probe
	float3 ibl_specular_probe = 0.0f;
	float probe_alpha = 0.0f;
	if (g_reflection_probe_available != 0)
	{
		float probe_radius = g_radius;
		float3 probe_position = g_float3;
		float3 box_min = probe_position - g_extents;
		float3 box_max = probe_position + g_extents;

		if (is_inside_box(surface.position, box_min, box_max))
		{
			float3 reflection = get_parallax_corrected_reflection(surface, probe_position, box_min, box_max);
			float4 probe_sample = tex_reflection_probe.SampleLevel(sampler_bilinear_clamp, reflection, 0.0f);
			ibl_specular_probe = probe_sample.rgb;
			probe_alpha = probe_sample.a;
		}
	}

    // Specular from SSR.
	float3 ibl_specular = color_ssr;

    // If there are no SSR data, fallback to to the reflection probe.
	ibl_specular = lerp(ibl_specular_probe, ibl_specular, ssr_alpha);

    // If there are no reflection probe data, fallback to the environment texture
	ibl_specular = lerp(ibl_specular_environment, ibl_specular, max(ssr_alpha, probe_alpha));

    // Modulate outcoming energy
	ibl_specular *= specular_energy;

    // SSAO
    //float bent_dot_l = 1.0f;
    //if (is_ssao_enabled() && use_ssao)
    //{
    //    float3 pixel_to_light = surface.normal;
    //    bent_dot_l            = saturate(dot(surface.bent_normal, world_to_view(pixel_to_light, false)));
    //}

    // Perfection achieved
	return float4(saturate_11(ibl_diffuse + ibl_specular), 0.0f);
}