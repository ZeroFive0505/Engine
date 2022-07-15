#include "Common.hlsl"

float3 uncharted_2(float3 x)
{
	float A = 0.15f;
	float B = 0.50f;
	float C = 0.10f;
	float D = 0.20f;
	float E = 0.02f;
	float F = 0.30f;
	float W = 11.2f;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

//== ACESFitted ===========================
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//  All code licensed under the MIT license
//=========================================

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 aces_mat_input =
{
	{ 0.59719, 0.35458, 0.04823 },
	{ 0.07600, 0.90834, 0.01566 },
	{ 0.02840, 0.13383, 0.83777 }
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 aces_mat_output =
{
	{ 1.60475, -0.53108, -0.07367 },
	{ -0.10208, 1.10813, -0.00605 },
	{ -0.00327, -0.07276, 1.07602 }
};

float3 RRTAndODTFit(float3 v)
{
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
	return a / b;
}

float3 aces_fitted(float3 color)
{
	color = mul(aces_mat_input, color);

	// Apply RRT and ODT
	color = RRTAndODTFit(color);

	color = mul(aces_mat_output, color);
	
	// Clamp to [0, 1]
	color = saturate(color);

	return color;
}

float3 matrix_movie(float3 keannu)
{
	static const float pow_a = 3.0f / 2.0f;
	static const float pow_b = 4.0f / 5.0f;

	return float3(pow(abs(keannu.r), pow_a), pow(abs(keannu.g), pow_b), pow(abs(keannu.b), pow_a));
}

float3 tone_map(float3 color)
{
	if (g_tonemapping == 0) // OFF
	{
        // Do nothing
	}
	else if (g_tonemapping == 1) // ACES
	{
        // attempting to match contrast levels
		color = pow(abs(color), 0.75f);
		color *= 1.07f;

		color = aces_fitted(color);
	}
	else if (g_tonemapping == 2) // REINHARD
	{
		color = reinhard(color);
	}
	else if (g_tonemapping == 3) // UNCHARTED 2
	{
		color = uncharted_2(color);
	}
	else if (g_tonemapping == 4) // MATRIX
	{
		color = matrix_movie(color);
	}
    
	return color;
}

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void mainCS(uint3 thread_id : SV_DispatchThreadID)
{
	// Out of bounds check
	if (any(int2(thread_id.xy) >= g_resolution_rt.xy))
		return;

	// Tone map and gamma correct
	float3 color = tex[thread_id.xy].rgb;
	tex_out_rgb[thread_id.xy] = gamma(tone_map(color));
}