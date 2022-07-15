#include "Common.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 positionWS : POSITIONT_WS;
};

PixelInputType mainVS(Vertex_PosUVNorTan input)
{
	PixelInputType output;
	
	input.position.w = 1.0f;
	output.positionWS = mul(input.position, g_transform).xyz;
	output.position = mul(float4(output.positionWS, 1.0f), g_view_projection_unjittered);
	output.normal = mul(input.normal, (float3x3) g_transform);
	output.uv = input.uv;
	
	return output;
}

float4 mainPS(PixelInputType input) : SV_TARGET
{
	float4 color = 0.0f;
	
#ifdef TRANSFORM
	float3 color_diffuse = g_float3.xyz;
	float3 color_ambient = color_diffuse * 0.3f;
	float3 color_specular = 1.0f;
	float3 lightPos = float3(10.0f, 10.0f, 10.0f);
	float3 normal = normalize(input.normal);
	float3 lightDir = normalize(lightPos - input.positionWS);
	float lambertian = max(dot(lightDir, normal), 0.0f);
	
	float specular = 0.0f;
	
	if (lambertian > 0.0f)
	{
		// Blinn phong
		float3 viewDir = normalize(g_camera_position - input.positionWS);
		float3 halfDir = normalize(lightDir + viewDir);
		float specAngle = max(dot(halfDir, normal), 0.0f);

		specular = pow(specAngle, 16.0f);
	}
	
	color = float4(color_ambient + lambertian * color_diffuse + color_specular * specular, 1.0f);
#endif

#ifdef OUTLINE
	float normal_threshold = 0.2f;
	
	float2 uv = world_to_uv_unjittered(input.positionWS.xyz);
	float scale = 1.0f;
	float halfScaleFloor = floor(scale * 0.5f);
	float halfScaleCeil = ceil(scale * 0.5f);

	// Sample X pattern
	float3 normal0 = get_normal(uv - g_texel_size * halfScaleFloor); // bottom left
	float3 normal1 = get_normal(uv + g_texel_size * halfScaleCeil); // top right
	float3 normal2 = get_normal(uv + float2(g_texel_size.x * halfScaleCeil, -g_texel_size.y * halfScaleFloor)); // bottom right
	float3 normal3 = get_normal(uv + float2(-g_texel_size.x * halfScaleFloor, g_texel_size.y * halfScaleCeil)); // top left
	
    // Compute edge normal
	float3 normalFiniteDifference0 = normal1 - normal0;
	float3 normalFiniteDifference1 = normal3 - normal2;
	float edge_normal = sqrt(dot(normalFiniteDifference0, normalFiniteDifference0) + dot(normalFiniteDifference1, normalFiniteDifference1));
	
	// Compute view direction bias
	float3 view = get_view_direction(uv);
	float3 normal = get_normal(uv);
	float view_dir_bias = dot(view, normal) * 0.5f + 0.5f;
	
	if (edge_normal * view_dir_bias < normal_threshold)
		discard;
	
	color = float4(0.6f, 0.6f, 1.0f, 1.0f);
#endif
	return color;
}