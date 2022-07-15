#include "Common.hlsl"

Pixel_PosUV mainVS(Vertex_PosUV input)
{
	Pixel_PosUV output;
	
	// position computation has to be an exact match to Gbuffer.hlsl
	input.position.w = 1.0f;
	output.position = mul(input.position, g_transform);
	output.position = mul(output.position, g_view_projection);

	output.uv = input.uv;
	
	return output;
}

void mainPS(Pixel_PosUV input)
{
	if (g_is_transparent_pass && tex_material_mask.Sample(sampler_anisotropic_wrap, input.uv).r <= ALPHA_THRESHOLD)
		discard;

	if (g_mat_color.a == 1.0f && tex_material_albedo.Sample(sampler_anisotropic_wrap, input.uv).a <= ALPHA_THRESHOLD)
		discard;
}