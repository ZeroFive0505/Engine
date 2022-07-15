#define FOG_REGULAR 1

#include "Common.hlsl"
#include "Fog.hlsl"

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
    // Out of bounds check.
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;

	Surface surface;
	surface.Build(thread_id.xy, true, false, false);

    // If this is a transparent pass, ignore all opaque pixels, and vice versa.
	bool early_exit_1 = is_opaque_pass() && surface.is_transparent();
	bool early_exit_2 = is_transparent_pass() && surface.is_opaque();
	bool early_exit_3 = is_opaque_pass() && surface.is_sky(); // During the opaque pass, allow sky pixels to be shaded.
	if (early_exit_1 || early_exit_2 || early_exit_3)
		return;

    // Compute fog.
	float3 fog = get_fog_factor(surface.position.y, surface.camera_to_pixel_length);

    // Modulate fog with ambient light
	float ambient_light = saturate(g_directional_light_intensity / 128000.0f);
	fog *= ambient_light * 0.25f;

	float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);

    // Draw the sky
	if (surface.is_sky())
	{
		color.rgb += tex_environment.SampleLevel(sampler_bilinear_clamp, direction_sphere_uv(surface.camera_to_pixel), 0).rgb;
		color.rgb *= ambient_light; // Modulate it's intensity in order to fake day/night.
		fog *= luminance(color.rgb);
	}
	else // Everything else.
	{
        // Light - Diffuse and Specular.
		float3 light_diffuse = tex_light_diffuse[thread_id.xy].rgb;
		float3 light_specular = tex_light_specular[thread_id.xy].rgb;

        // Light - Refraction
		float3 light_refraction = 0.0f;
		if (surface.is_transparent())
		{
            // Compute refraction UV offset.
			float ior = 1.5; // glass
			float scale = 0.03f;
			float distance_falloff = clamp(1.0f / world_to_view(surface.position).z, -3.0f, 3.0f);
			float2 refraction_normal = world_to_view(surface.normal.xyz, false).xy;
			float2 refraction_uv_offset = refraction_normal * distance_falloff * scale * max(0.0f, ior - 1.0f);

            // Only refract what's behind the surface.
			float depth_surface = get_linear_depth(surface.depth);
			float depth_surface_refracted = get_linear_depth(surface.uv + refraction_uv_offset);
			float is_behind = step(depth_surface - 0.02f, depth_surface_refracted); // step does a >=, but when the depth is equal, we still want refract, so we use a bias of 0.02.

            // Refraction from ALDI.
			float roughness2 = surface.roughness * surface.roughness;
			float mip_level = lerp(0, g_frame_mip_count, roughness2);
			light_refraction = tex_frame.SampleLevel(sampler_trilinear_clamp, surface.uv + refraction_uv_offset * is_behind, mip_level).rgb;
		}
        
        // Compose everything.
		float3 light_ds = light_diffuse * surface.albedo + light_specular + surface.emissive;
		color.rgb += lerp(light_ds, light_refraction, 1.0f - surface.alpha);
	}

    // Accumulate fog.
	color.rgb += fog; // regular
	color.rgb += tex_light_volumetric[thread_id.xy].rgb; // volumetric

	tex_out_rgba[thread_id.xy] = saturate_16(color);
}
