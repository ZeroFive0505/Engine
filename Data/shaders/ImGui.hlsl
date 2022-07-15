#include "Common.hlsl"

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

PS_INPUT mainVS(Vertex_Pos2DUVColor input)
{
	PS_INPUT output;
	output.position = mul(g_transform, float4(input.position.xy, 0.0f, 1.0f));
	output.color = input.color;
	output.uv = input.uv;
	return output;
}

float4 visualise_texture(float4 color_in)
{
	float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);

	// Set requested channels channels
	{
		if (texture_channel_r())
		{
			color.r = color_in.r;
		}
		
		if (texture_channel_g())
		{
			color.g = color_in.g;
		}
		
		if (texture_channel_b())
		{
			color.b = color_in.b;
		}
		
		if (texture_channel_a())
		{
			color.a = color_in.a;
		}
	}
	
	if (texture_gamma_correction())
	{
		color.rgb = gamma(color.rgb);
	}

	if (texture_abs())
	{
		color = abs(color);
	}

	if (texture_pack())
	{
		color.rgb = pack(color.rgb);
	}

	if (texture_boost())
	{
		color.rgb *= 10.0f;
	}

	return color;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
	float4 color_vertex = input.color;
	float4 color_texture = tex.Sample(sampler_bilinear_wrap, input.uv);

	// Render targets can be visualized in various ways.
	if (texture_visualise())
	{
		if (texture_sample_point())
		{
			color_texture = tex.Sample(sampler_point_wrap, input.uv);
		}
		
		color_texture = visualise_texture(color_texture);
	}
	
	return color_vertex * color_texture;
}