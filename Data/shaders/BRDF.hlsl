#include "Common.hlsl"

/*------------------------------------------------------------------------------
    Fresnel, visibility and normal distribution functions
------------------------------------------------------------------------------*/

float3 F_Schlick(const float3 f0, float f90, float v_dot_h)
{
    // Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
	return f0 + (f90 - f0) * pow(1.0 - v_dot_h, 5.0);
}

float3 F_Schlick(const float3 f0, float v_dot_h)
{
	float f = pow(1.0 - v_dot_h, 5.0);
	return f + f0 * (1.0 - f);
}

float3 F_Schlick_Roughness(float3 f0, float cosTheta, float roughness)
{
	float3 a = 1.0 - roughness;
	return f0 + (max(a, f0) - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// Smith term for GGX
// [Smith 1967, "Geometrical shadowing of a random rough surface"]
inline float V_Smith(float a2, float n_dot_v, float n_dot_l)
{
	float Vis_SmithV = n_dot_v + sqrt(n_dot_v * (n_dot_v - n_dot_v * a2) + a2);
	float Vis_SmithL = n_dot_l + sqrt(n_dot_l * (n_dot_l - n_dot_l * a2) + a2);
	return rcp(Vis_SmithV * Vis_SmithL);
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
inline float V_SmithJointApprox(float a, float n_dot_v, float n_dot_l)
{
	float Vis_SmithV = n_dot_l * (n_dot_v * (1 - a) + a);
	float Vis_SmithL = n_dot_v * (n_dot_l * (1 - a) + a);
	return saturate_16(0.5 * rcp(Vis_SmithV + Vis_SmithL));
}

float V_GGX_anisotropic_2cos(float cos_theta_m, float alpha_x, float alpha_y, float cos_phi, float sin_phi)
{
	float cos2 = cos_theta_m * cos_theta_m;
	float sin2 = (1.0 - cos2);
	float s_x = alpha_x * cos_phi;
	float s_y = alpha_y * sin_phi;
	return 1.0 / max(cos_theta_m + sqrt(cos2 + (s_x * s_x + s_y * s_y) * sin2), 0.001);
}

// [Kelemen 2001, "A microfacet based coupled specular-matte brdf model with importance sampling"]
float V_Kelemen(float v_dot_h)
{
    // constant to prevent NaN
	return rcp(4 * v_dot_h * v_dot_h + 1e-5);
}

// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
float V_Neubelt(float n_dot_v, float n_dot_l)
{
	return saturate_16(1.0 / (4.0 * (n_dot_l + n_dot_v - n_dot_l * n_dot_v)));
}

// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX(float a2, float n_dot_h)
{
	float d = (n_dot_h * a2 - n_dot_h) * n_dot_h + 1; // 2 mad
	return a2 / (PI * d * d); // 4 mul, 1 rcp
}

float D_GGX_Anisotropic(float cos_theta_m, float alpha_x, float alpha_y, float cos_phi, float sin_phi)
{
	float cos2 = cos_theta_m * cos_theta_m;
	float sin2 = (1.0 - cos2);
	float r_x = cos_phi / alpha_x;
	float r_y = sin_phi / alpha_y;
	float d = cos2 + sin2 * (r_x * r_x + r_y * r_y);
	return saturate_16(1.0 / (PI * alpha_x * alpha_y * d * d));
}

float D_Charlie(float roughness, float NoH)
{
    // Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
	float invAlpha = 1.0 / roughness;
	float cos2h = NoH * NoH;
	float sin2h = max(1.0 - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 > 0 in fp16
	return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}

/*------------------------------------------------------------------------------
    Diffuse
------------------------------------------------------------------------------*/

float3 Diffuse_Lambert(float3 diffuse_color)
{
	return diffuse_color * (1 / PI);
}

// [Burley 2012, "Physically-Based Shading at Disney"]
float3 Diffuse_Burley(float3 diffuse_color, float Roughness, float NoV, float NoL, float VoH)
{
	float FD90 = 0.5 + 2 * VoH * VoH * Roughness;
	float FdV = 1 + (FD90 - 1) * pow(1 - NoV, 5);
	float FdL = 1 + (FD90 - 1) * pow(1 - NoL, 5);
	return diffuse_color * ((1 / PI) * FdV * FdL);
}

// Diffuse - [Gotanda 2012, "Beyond a Simple Physically Based Blinn-Phong Model in Real-Time"]
float3 Diffuse_OrenNayar(float3 diffuse_color, float Roughness, float NoV, float NoL, float VoH)
{
	float a = Roughness * Roughness;
	float s = a; // ( 1.29 + 0.5 * a );
	float s2 = s * s;
	float VoL = 2 * VoH * VoH - 1; // double angle identity
	float Cosri = VoL - NoV * NoL;
	float C1 = 1 - 0.5 * s2 / (s2 + 0.33);
	float C2 = 0.45 * s2 / (s2 + 0.09) * Cosri * (Cosri >= 0 ? rcp(max(NoL, NoV + 0.0001f)) : 1);
	return diffuse_color / PI * (C1 + C2) * (1 + Roughness * 0.5);
}

float3 BRDF_Diffuse(float3 albedo, float roughhness, float n_dot_v, float n_dot_l, float v_dot_h)
{
	return Diffuse_OrenNayar(albedo, roughhness, n_dot_v, n_dot_l, v_dot_h);
}

float3 BRDF_Diffuse(Surface surface, float n_dot_v, float n_dot_l, float v_dot_h)
{
	return BRDF_Diffuse(surface.albedo.rgb, surface.roughness, n_dot_v, n_dot_l, v_dot_h);
}

/*------------------------------------------------------------------------------
    Specular
------------------------------------------------------------------------------*/

float3 BRDF_Specular_Isotropic(float roughness, float metallic, float3 F0, float n_dot_v, float n_dot_l, float n_dot_h, float v_dot_h, float l_dot_h, inout float3 diffuse_energy, inout float3 specular_energy)
{
	float a = pow2(roughness);
	float a2 = pow4(roughness);

	float V = V_SmithJointApprox(a, n_dot_v, n_dot_l);
	float D = D_GGX(a2, n_dot_h);
	float3 F = F_Schlick(F0, v_dot_h);

	diffuse_energy *= compute_diffuse_energy(F, metallic);
	specular_energy *= F;

	return D * V * F;
}

float3 BRDF_Specular_Isotropic(Surface surface, float n_dot_v, float n_dot_l, float n_dot_h, float v_dot_h, float l_dot_h, inout float3 diffuse_energy, inout float3 specular_energy)
{
	return BRDF_Specular_Isotropic(surface.roughness, surface.metallic, surface.F0, n_dot_v, n_dot_l, n_dot_h, v_dot_h, l_dot_h, diffuse_energy, specular_energy);
}

inline float3 BRDF_Specular_Anisotropic(Surface surface, float3 v, float3 l, float3 h, float n_dot_v, float n_dot_l, float n_dot_h, float l_dot_h, inout float3 diffuse_energy, inout float3 specular_energy)
{
    // Construct TBN from the normal
	float3 t, b;
	find_best_axis_vectors(surface.normal, t, b);
	float3x3 TBN = float3x3(t, b, surface.normal);

    // Rotate tangent and bitagent
	float rotation = max(surface.anisotropic_rotation * PI2, FLT_MIN); // convert material property to a full rotation
	float2 direction = float2(cos(rotation), sin(rotation)); // convert rotation to direction
	t = normalize(mul(float3(direction, 0.0f), TBN).xyz); // compute direction derived tangent
	b = normalize(cross(surface.normal, t)); // re-compute bitangent

	float alpha_ggx = surface.roughness;
	float aspect = sqrt(1.0 - surface.anisotropic * 0.9);
	float ax = alpha_ggx / aspect;
	float ay = alpha_ggx * aspect;
	float XdotH = dot(t, h);
	float YdotH = dot(b, h);
    
    // specular anisotropic BRDF
	float D = D_GGX_Anisotropic(n_dot_h, ax, ay, XdotH, YdotH);
	float V = V_GGX_anisotropic_2cos(n_dot_v, ax, ay, XdotH, YdotH) * V_GGX_anisotropic_2cos(n_dot_v, ax, ay, XdotH, YdotH);
	float f90 = saturate(dot(surface.F0, 50.0 * 0.33));
	float3 F = F_Schlick(surface.F0, f90, l_dot_h);

	diffuse_energy *= compute_diffuse_energy(F, surface.metallic);
	specular_energy *= F;
    
	return D * V * F;
}

inline float3 BRDF_Specular_Clearcoat(Surface surface, float n_dot_h, float v_dot_h, inout float3 diffuse_energy, inout float3 specular_energy)
{
	float a2 = pow4(surface.roughness);
    
	float D = D_GGX(a2, n_dot_h);
	float V = V_Kelemen(v_dot_h);
	float3 F = F_Schlick(0.04, 1.0, v_dot_h) * surface.clearcoat;

	diffuse_energy *= compute_diffuse_energy(F, surface.metallic);
	specular_energy *= F;

	return D * V * F;
}

inline float3 BRDF_Specular_Sheen(Surface surface, float n_dot_v, float n_dot_l, float n_dot_h, inout float3 diffuse_energy, inout float3 specular_energy)
{
    // Mix between white and using base color for sheen reflection
	float tint = surface.sheen_tint * surface.sheen_tint;
	float3 f0 = lerp(1.0f, surface.F0, tint);
    
	float D = D_Charlie(surface.roughness, n_dot_h);
	float V = V_Neubelt(n_dot_v, n_dot_l);
	float3 F = f0 * surface.sheen;

	diffuse_energy *= compute_diffuse_energy(F, surface.metallic);
	specular_energy *= F;

	return D * V * F;
}