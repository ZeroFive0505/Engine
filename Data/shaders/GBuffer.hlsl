#include "Common.hlsl"
#include "Parallax_mapping.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 position_ss_current : SCREEN_POS;
	float4 position_ss_previous : SCREEN_POS_PREVIOUS;
};

struct PixelOutputType
{
	float4 albedo : SV_Target0;
	float4 normal : SV_Target1;
	float4 material : SV_Target2;
	float2 velocity : SV_Target3;
};

PixelInputType mainVS(Vertex_PosUVNorTan input)
{
	PixelInputType output;
	
	// position computation has to be an exact match to depth_prepass.hlsl
	input.position.w = 1.0f;
	output.position = mul(input.position, g_transform);
	output.position = mul(output.position, g_view_projection);
	
	output.position_ss_current = output.position;
	output.position_ss_previous = mul(input.position, g_transform_previous);
	output.position_ss_previous = mul(output.position_ss_previous, g_view_projection_previous);
	output.normal = normalize(mul(input.normal, (float3x3) g_transform)).xyz;
	output.tangent = normalize(mul(input.tangent, (float3x3) g_transform)).xyz;
	output.uv = input.uv;

	return output;
}

PixelOutputType mainPS(PixelInputType input)
{
	// Velocity
	float2 position_uv_current = ndc_to_uv((input.position_ss_current.xy / input.position_ss_current.w) - g_taa_jitter_current);
	float2 position_uv_previous = ndc_to_uv((input.position_ss_previous.xy / input.position_ss_previous.w) - g_taa_jitter_previous);
	float2 velocity_uv = position_uv_current - position_uv_previous;
	
	// TBN
	float3x3 TBN = 0.0f;
	if (has_texture_height() || has_texture_normal())
	{
		TBN = makeTBN(input.normal, input.tangent);
	}
	
	// Compute UV coordinates.
	float2 taa_jitter_uv_space = ddx_fine(input.uv) * g_taa_jitter_current.x + ddy_fine(input.uv) * g_taa_jitter_current.y;
	float2 uv = input.uv - ((float) is_taa_enabled() * taa_jitter_uv_space); // If TAA is enabled, remove jitter (less blurring).
	uv = float2(uv.x * g_mat_tiling.x + g_mat_offset.x, uv.y * g_mat_tiling.y + g_mat_offset.y); // Apply material tiling and offset.
	
	// Parallax mapping
	if (has_texture_height())
	{
		float height_scale = g_mat_height * 0.04f;
		float3 camera_to_pixel = normalize(g_camera_position - input.position.xyz);
		uv = ParallaxMapping(tex_material_height, sampler_anisotropic_wrap, uv, camera_to_pixel, TBN, height_scale);
	}
	
	// Alpha mask
	float alpha_mask = 1.0f;
	if (has_texture_alpha_mask())
	{
		alpha_mask = tex_material_mask.Sample(sampler_anisotropic_wrap, uv).r;
	}
	
	// Albedo
	float4 albedo = g_mat_color;
	if (has_texture_albedo())
	{
		float4 albedo_sample = tex_material_albedo.Sample(sampler_anisotropic_wrap, uv);

		// Read albedo's alpha channel as an alpha mask as well
		alpha_mask = min(alpha_mask, albedo_sample.a);
		albedo_sample.a = 1.0f;
		
		albedo_sample.rgb = degamma(albedo_sample.rgb);
		albedo *= albedo_sample;
	}
	
	// Discard masked pixels
	if (alpha_mask <= ALPHA_THRESHOLD)
		discard;
	
	// Roughness
	float roughness = g_mat_roughness;
	if (has_texture_roughness())
	{
		roughness *= tex_material_roughness.Sample(sampler_anisotropic_wrap, uv).r;
	}
	
	// Metallic
	float metallic = g_mat_metallic;
	if (has_texture_metallic())
	{
		metallic *= tex_material_metallic.Sample(sampler_anisotropic_wrap, uv).r;
	}
	
	// Normal
	float3 normal = input.normal.xyz;
	
	if (has_texture_normal())
	{
		// Get tangent space normal and apply the user defined intensity. Then transform it to the world space.
		float3 tangent_normal = normalize(unpack(tex_material_normal.Sample(sampler_anisotropic_wrap, uv).rgb));
		float normal_intensity = clamp(g_mat_normal, 0.012f, g_mat_normal);
		tangent_normal.xy *= saturate(normal_intensity);
		normal = normalize(mul(tangent_normal, TBN).xyz);
	}
	
	// Occlusion
	float occlusion = 1.0f;
	if (has_texture_occlusion())
	{
		occlusion = tex_material_occlusion.Sample(sampler_anisotropic_wrap, uv).r;
	}
	
	// Emission
	float emission = 0.0f;
	if (has_texture_emissive())
	{
		emission = luminance(tex_material_emission.Sample(sampler_anisotropic_wrap, uv).rgb);
	}
	
    // Specular anti-aliasing
	{
		static const float strength = 1.0f;
		static const float max_roughness_gain = 0.02f;

		float roughness2 = roughness * roughness;
		float3 dndu = ddx(normal), dndv = ddy(normal);
		float variance = (dot(dndu, dndu) + dot(dndv, dndv));
		float kernelRoughness2 = min(variance * strength, max_roughness_gain);
		float filteredRoughness2 = saturate(roughness2 + kernelRoughness2);
		roughness = fast_sqrt(filteredRoughness2);
	}
	
	// Write to G-Buffer
	PixelOutputType g_buffer;
	g_buffer.albedo = albedo;
	g_buffer.normal = float4(normal, pack_uint32_to_float16(g_mat_id));
	g_buffer.material = float4(roughness, metallic, emission, occlusion);
	g_buffer.velocity = velocity_uv;
	
	return g_buffer;
}