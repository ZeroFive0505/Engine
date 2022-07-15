
// Low freq - Updates once per frame
cbuffer BufferFrame : register(b0)
{
	matrix g_view;
	matrix g_projection;
	matrix g_projection_inverted;
	matrix g_projection_orthographic;
	matrix g_view_projection;
	matrix g_view_projection_inverted;
	matrix g_view_projection_orthographic;
	matrix g_view_projection_unjittered;
	matrix g_view_projection_previous;
	
	float g_delta_time;
	float g_time;
	uint g_frame;
	float g_camera_aperture;

	float g_camera_shutter_speed;
	float g_camera_iso;
	float g_camera_near;
	float g_camera_far;
	
	float3 g_camera_position;
	float g_bloom_intensity;
	
	float g_sharpen_strength;
	float3 g_camera_direction;
	
	float g_gamma;
	uint g_tonemapping;
	float g_directional_light_intensity;
	float g_shadow_resolution;
	
	float2 g_resolution_render;
	float2 g_resolution_output;
	
	float2 g_taa_jitter_current;
	float2 g_taa_jitter_previous;
	
	float g_fog_density;
	uint g_options;
	uint g_frame_mip_count;
	uint g_ssr_mip_count;
	
	float2 g_resolution_environment;
	float2 g_padding;
}


// Medium freq - Updates per render pass
cbuffer BufferUber : register(b1)
{
	matrix g_transform;
	matrix g_transform_previous;
	
	float3 g_float3;
	float g_blur_sigma;
	
	float2 g_blur_direction;
	float2 g_resolution_rt;
	
	float2 g_resolution_in;
	uint g_texture_flags;
	float g_radius;
	
	float4 g_mat_color;
	
	float2 g_mat_tiling;
	float2 g_mat_offset;
	
	float g_mat_roughness;
	float g_mat_metallic;
	float g_mat_normal;
	float g_mat_height;
	
	uint g_mat_id;
	uint g_mat_textures;
	uint g_is_transparent_pass;
	uint g_mip_count;

	float3 g_extents;
	uint g_work_group_count;

	uint g_reflection_probe_available;
	float3 g_padding2;
}

// High freq - Updates per light
cbuffer LightBuffer : register(b2)
{
	matrix cb_light_view_projection[6];
	float4 cb_light_intensity_range_angle_bias;
	float3 cb_light_color;
	float cb_light_normal_bias;
	float4 cb_light_position;
	float4 cb_light_direction;
	uint cb_options;
	uint3 cb_padding;
}

// Low freq - Updates once per frame
static const int g_max_materials = 1024;
cbuffer BufferMaterial : register(b3)
{
	float4 mat_clearcoat_clearcoatRough_aniso_anisoRot[g_max_materials];
	float4 mat_sheen_sheenTint_pad[g_max_materials];
}

// Options g-buffer textures
bool has_texture_height()
{
	return g_mat_textures & uint(1U << 0);
}
bool has_texture_normal()
{
	return g_mat_textures & uint(1U << 1);
}
bool has_texture_albedo()
{
	return g_mat_textures & uint(1U << 2);
}
bool has_texture_roughness()
{
	return g_mat_textures & uint(1U << 3);
}
bool has_texture_metallic()
{
	return g_mat_textures & uint(1U << 4);
}
bool has_texture_alpha_mask()
{
	return g_mat_textures & uint(1U << 5);
}
bool has_texture_emissive()
{
	return g_mat_textures & uint(1U << 6);
}
bool has_texture_occlusion()
{
	return g_mat_textures & uint(1U << 7);
}

// Options lighting
bool light_is_directional()
{
	return cb_options & uint(1U << 0);
}
bool light_is_point()
{
	return cb_options & uint(1U << 1);
}
bool light_is_spot()
{
	return cb_options & uint(1U << 2);
}
bool light_has_shadows()
{
	return cb_options & uint(1U << 3);
}
bool light_has_shadows_transparent()
{
	return cb_options & uint(1U << 4);
}
bool light_has_shadows_screen_space()
{
	return cb_options & uint(1U << 5);
}
bool light_is_volumetric()
{
	return cb_options & uint(1U << 6);
}

// Options passes
bool is_taa_enabled()
{
	return any(g_taa_jitter_current);
}
bool is_ssr_enabled()
{
	return g_options & uint(1U << 0);
}
bool is_taa_upsampling_enabled()
{
	return g_options & uint(1U << 1);
}
bool is_ssao_enabled()
{
	return g_options & uint(1U << 2);
}
bool is_volumetric_fog_enabled()
{
	return g_options & uint(1U << 3);
}
bool is_screen_space_shadows_enabled()
{
	return g_options & uint(1U << 4);
}
bool is_ssao_gi_enabled()
{
	return g_options & uint(1U << 5);
}

// Options texture visualisation
bool texture_visualise()
{
	return g_texture_flags & uint(1U << 12);
}
bool texture_pack()
{
	return g_texture_flags & uint(1U << 13);
}
bool texture_gamma_correction()
{
	return g_texture_flags & uint(1U << 14);
}
bool texture_boost()
{
	return g_texture_flags & uint(1U << 15);
}
bool texture_abs()
{
	return g_texture_flags & uint(1U << 16);
}
bool texture_channel_r()
{
	return g_texture_flags & uint(1U << 17);
}
bool texture_channel_g()
{
	return g_texture_flags & uint(1U << 18);
}
bool texture_channel_b()
{
	return g_texture_flags & uint(1U << 19);
}
bool texture_channel_a()
{
	return g_texture_flags & uint(1U << 20);
}
bool texture_sample_point()
{
	return g_texture_flags & uint(1U << 21);
}

// Misc
bool is_opaque_pass()
{
	return g_is_transparent_pass == 0;
}
bool is_transparent_pass()
{
	return g_is_transparent_pass == 1;
}
