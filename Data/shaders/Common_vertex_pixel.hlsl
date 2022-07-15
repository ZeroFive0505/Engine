
// Vertex
struct Vertex_Pos
{
	float4 position : POSITION0;
};

struct Vertex_PosUV
{
	float4 position : POSITION0;
	float2 uv : TEXCOORD0;
};

struct Vertex_PosColor
{
	float4 position : POSITION0;
	float4 color : COLOR0;
};

struct Vertex_PosUVNorTan
{
	float4 position : POSITION0;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL0;
	float3 tangent : TANGENT0;
};

struct Vertex_Pos2DUVColor
{
	float2 position : POSITION0;
	float2 uv : TEXCOORD0;
	float4 color : COLOR0;
};

// Pixel
struct Pixel_Pos
{
	float4 position : SV_POSITION;
};

struct Pixel_PosUV
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

struct Pixel_PosColor
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

struct Pixel_PosNor
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

struct Pixel_PosUVNorTan
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};