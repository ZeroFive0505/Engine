#include "Common.hlsl"

#ifndef COMMON_STRUCT
#define COMMON_STRUCT

struct Surface
{
	// Properties
	float3 albedo;
	float alpha;
	float roughness;
	float metallic;
	float clearcoat;
	float clearcoat_roughness;
	float anisotropic;
	float anisotropic_rotation;
	float sheen;
	float sheen_tint;
	float3 occlusion;
	float3 gi;
	float3 bent_normal;
	float3 emissive;
	float3 F0;
	int id;
	float2 uv;
	float depth;
	float3 position;
	float3 normal;
	float3 camera_to_pixel;
	float camera_to_pixel_length;
	
	 // Activision GTAO paper: https://www.activision.com/cdn/research/s2016_pbs_activision_occlusion.pptx
	float3 multi_bounce_ao(float visibility, float3 albedo)
	{
		float3 a = 2.0404 * albedo - 0.3324;
		float3 b = -4.7951 * albedo + 0.6417;
		float3 c = 2.7552 * albedo + 0.6903;
		float x = visibility;
		return max(x, ((x * a + b) * x + c) * x);
	}
	
	void Build(uint2 position_screen, bool use_albedo, bool use_ssao, bool replace_color_with_one)
	{
        // Sample render targets
		float4 sample_albedo = use_albedo ? tex_albedo[position_screen] : 0.0f;
		float4 sample_normal = tex_normal[position_screen];
		float4 sample_material = tex_material[position_screen];
		float sample_depth = get_depth(position_screen);

        // Misc
		uv = (position_screen + 0.5f) / g_resolution_rt;
		depth = sample_depth;
		normal = sample_normal.xyz;
		id = unpack_float16_to_uint32(sample_normal.a);

		albedo = replace_color_with_one ? 1.0f : sample_albedo.rgb;
		alpha = sample_albedo.a;
		roughness = sample_material.r;
		metallic = sample_material.g;
		emissive = sample_material.b * (use_albedo ? albedo : 1.0f) * 10.0f;
		F0 = lerp(0.04f, albedo, metallic);
		clearcoat = mat_clearcoat_clearcoatRough_aniso_anisoRot[id].x;
		clearcoat_roughness = mat_clearcoat_clearcoatRough_aniso_anisoRot[id].y;
		anisotropic = mat_clearcoat_clearcoatRough_aniso_anisoRot[id].z;
		anisotropic_rotation = mat_clearcoat_clearcoatRough_aniso_anisoRot[id].w;
		sheen = mat_sheen_sheenTint_pad[id].x;
		sheen_tint = mat_sheen_sheenTint_pad[id].y;

        // Occlusion + GI
        {
			occlusion = 1.0f;
			gi = 0.0f;
			bent_normal = normal;

			if (is_ssao_enabled() && use_ssao)
			{
                // Sample ssao texture
				float4 ssao = tex_ssao[position_screen];
				bent_normal = ssao.xyz;
				occlusion = ssao.a;

                // Combine occlusion with material occlusion (baked texture).
				occlusion = min(sample_material.a, occlusion);
				occlusion = multi_bounce_ao(ssao.a, albedo);

                // If ssao gi is not enabled, approximate some light bouncing
				gi = is_ssao_gi_enabled() ? tex_ssao_gi[position_screen].rgb : 0.0f;
			}
		}

		position = get_position_ws_from_depth(uv, depth);
		camera_to_pixel = position - g_camera_position.xyz;
		camera_to_pixel_length = length(camera_to_pixel);
		camera_to_pixel = normalize(camera_to_pixel);
	}

	bool is_sky()
	{
		return id == 0;
	}
	
	bool is_transparent()
	{
		return alpha != 1.0f;
	}
	
	bool is_opaque()
	{
		return alpha == 1.0f;
	}
};

struct Light
{
    // Properties
	float3 color;
	float3 position;
	float intensity;
	float3 to_pixel;
	float3 forward;
	float distance_to_pixel;
	float angle;
	float bias;
	float normal_bias;
	float near;
	float far;
	float3 radiance;
	float n_dot_l;
	uint array_size;
	float attenuation;

    // attenuation functions are derived from Frostbite
    // https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/course-notes-moving-frostbite-to-pbr-v2.pdf

    // Attenuation over distance
	float compute_attenuation_distance(const float3 surface_position)
	{
		float distance_to_pixel = length(surface_position - position);
		float attenuation = saturate(1.0f - distance_to_pixel / far);
		return attenuation * attenuation;
	}

    // Attenuation over angle (approaching the outer cone)
	float compute_attenuation_angle()
	{
		float cos_outer = cos(angle);
		float cos_inner = cos(angle * 0.9f);
		float cos_outer_squared = cos_outer * cos_outer;
		float scale = 1.0f / max(0.001f, cos_inner - cos_outer);
		float offset = -cos_outer * scale;

		float cd = dot(to_pixel, forward);
		float attenuation = saturate(cd * scale + offset);
		return attenuation * attenuation;
	}

    // Final attenuation for all suported lights
	float compute_attenuation(const float3 surface_position)
	{
		float attenuation = 0.0f;
        
		if (light_is_directional())
		{
			attenuation = saturate(dot(-forward.xyz, float3(0.0f, 1.0f, 0.0f)));
		}
		else if (light_is_point())
		{
			attenuation = compute_attenuation_distance(surface_position);
		}
		else if (light_is_spot())
		{
			attenuation = compute_attenuation_distance(surface_position) * compute_attenuation_angle();
		}

		return attenuation;
	}

	float3 compute_direction(float3 light_position, float3 fragment_position)
	{
		float3 direction = 0.0f;
        
		if (light_is_directional())
		{
			direction = normalize(forward.xyz);
		}
		else if (light_is_point())
		{
			direction = normalize(fragment_position - light_position);
		}
		else if (light_is_spot())
		{
			direction = normalize(fragment_position - light_position);
		}

		return direction;
	}

	void Build(float3 surface_position, float3 surface_normal, float3 surface_bent_normal)
	{
		color = cb_light_color.rgb;
		position = cb_light_position.xyz;
		intensity = cb_light_intensity_range_angle_bias.x;
		far = cb_light_intensity_range_angle_bias.y;
		angle = cb_light_intensity_range_angle_bias.z;
		bias = cb_light_intensity_range_angle_bias.w;
		forward = cb_light_direction.xyz;
		normal_bias = cb_light_normal_bias;
		near = 0.1f;
		distance_to_pixel = length(surface_position - position);
		to_pixel = compute_direction(position, surface_position);
		n_dot_l = saturate(dot(surface_normal, -to_pixel)); // Pre-compute n_dot_l since it's used in many places
		attenuation = compute_attenuation(surface_position);
		array_size = light_is_directional() ? 4 : 1;
        
        // Apply SSAO
		if (is_ssao_enabled())
		{
			float bent_dot_l = 1.0f - saturate(dot(surface_bent_normal, world_to_view(-to_pixel, false)));
			n_dot_l = min(n_dot_l, bent_dot_l);
		}

		radiance = color * intensity * attenuation * n_dot_l;
	}

	void Build(Surface surface)
	{
		Build(surface.position, surface.normal, surface.bent_normal);
	}
};

#endif