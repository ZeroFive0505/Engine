// Material
Texture2D tex_material_albedo : register(t0);
Texture2D tex_material_roughness : register(t1);
Texture2D tex_material_metallic : register(t2);
Texture2D tex_material_normal : register(t3);
Texture2D tex_material_height : register(t4);
Texture2D tex_material_occlusion : register(t5);
Texture2D tex_material_emission : register(t6);
Texture2D tex_material_mask : register(t7);

// G-buffer
Texture2D tex_albedo : register(t8);
Texture2D tex_normal : register(t9);
Texture2D tex_material : register(t10);
Texture2D tex_velocity : register(t11);
Texture2D tex_velocity_previous : register(t12);
Texture2D tex_depth : register(t13);

Texture2D tex_light_diffuse : register(t14);
Texture2D tex_light_diffuse_transparent : register(t15);
Texture2D tex_light_specular : register(t16);
Texture2D tex_light_specular_transparent : register(t17);
Texture2D tex_light_volumetric : register(t18);

// Light depth/color maps
Texture2DArray tex_light_directional_depth : register(t19);
Texture2DArray tex_light_directional_color : register(t20);
TextureCube tex_light_point_depth : register(t21);
TextureCube tex_light_point_color : register(t22);
Texture2D tex_light_spot_depth : register(t23);
Texture2D tex_light_spot_color : register(t24);

// Noise
Texture2D tex_noise_normal : register(t25);
Texture2DArray tex_noise_blue : register(t26);

// Misc
Texture2D tex_lut_ibl : register(t27);
Texture2D tex_environment : register(t28);
Texture2D tex_ssao : register(t29);
Texture2D tex_ssao_gi : register(t30);
Texture2D tex_ssr : register(t31);
Texture2D tex_frame : register(t32);
Texture2D tex : register(t33);
Texture2D tex2 : register(t34);
Texture2D tex_font_atlas : register(t35);
TextureCube tex_reflection_probe : register(t36);

// RWTexture2D
RWTexture2D<float> tex_out_r : register(u0);
RWTexture2D<float2> tex_out_rg : register(u1);
RWTexture2D<float3> tex_out_rgb : register(u2);
RWTexture2D<float3> tex_out_rgb2 : register(u3);
RWTexture2D<float3> tex_out_rgb3 : register(u4);
RWTexture2D<float4> tex_out_rgba : register(u5);
RWTexture2D<float4> tex_out_rgba2 : register(u6);
globallycoherent RWTexture2D<float4> tex_out_rgba_mips[12] : register(u7);

// Atomic counter
globallycoherent RWStructuredBuffer<uint> g_atomic_counter : register(u19); // u7 + 12 mips = u19