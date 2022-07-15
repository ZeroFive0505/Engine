#include "Common.hlsl"

Pixel_PosUV mainVS(uint vertexID : SV_VertexID)
{
	Pixel_PosUV output;
	
	output.uv = float2((vertexID << 1) & 2, vertexID & 2);
	output.position = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);

	return output;
}