#include "Common.hlsl"

#define A_GPU
#define A_HLSL
#define FSR_EASU_F
#define FSR_RCAS_F

#include "ffx_a.h"

// Functions required by ffx_fsr1.h
AF4 FsrEasuRF(AF2 p)
{
	AF4 res = tex.GatherRed(sampler_bilinear_clamp, p, int2(0, 0));
	return res;
}

AF4 FsrEasuGF(AF2 p)
{
	AF4 res = tex.GatherGreen(sampler_bilinear_clamp, p, int2(0, 0));
	return res;
}

AF4 FsrEasuBF(AF2 p)
{
	AF4 res = tex.GatherBlue(sampler_bilinear_clamp, p, int2(0, 0));
	return res;
}

AF4 FsrRcasLoadF(ASU2 p)
{
	return tex.Load(int3(ASU2(p), 0));
}

void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b)
{
}

#include "ffx_fsr1.h"

// EDGE ADAPTIVE SPATIAL UPSAMPLING
void filter_easu(int2 pos, AU4 const0, AU4 const1, AU4 const2, AU4 const3)
{
	AF3 color = AF3(0.0f, 0.0f, 0.0f);
	FsrEasuF(color, pos, const0, const1, const2, const3);
	tex_out_rgb[pos] = color;
}

// ROBUST CONTRAST ADAPTIVE SHARPENING
void filter_rcas(int2 pos, AU4 const0)
{
	AF3 color;
	FsrRcasF(color.r, color.g, color.b, pos, const0);
	tex_out_rgb[pos] = color;
}

void setup(inout AU4 const0, inout AU4 const1, inout AU4 const2, inout AU4 const3)
{
#if UPSAMPLE
	    FsrEasuCon
        (
            const0,
            const1,
            const2,
            const3,
            g_resolution_render.x, // This the rendered image resolution being upscaled
            g_resolution_render.y,
            g_resolution_render.x, // This is the resolution of the resource containing the input image (useful for dynamic resolution)
            g_resolution_render.y,
            g_resolution_output.x, // This is the display resolution which the input image gets upscaled to
            g_resolution_output.y 
        );
#endif

#if SHARPEN
        const float sharpness = 0.2f; // AMDs recommended value - Goes from 0.0 (sharpest) to about 2.0
        FsrRcasCon(const0, sharpness);
#endif
}

void filter(int2 pos, AU4 const0, AU4 const1, AU4 const2, AU4 const3)
{
#if UPSAMPLE
    filter_easu(pos, const0, const1, const2, const3);
#endif

#if SHARPEN 
    filter_rcas(pos, const0);
#endif
}

[numthreads(64, 1, 1)]
void mainCS(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID)
{
    // Setup
    AU4 const0 = AU4(0, 0, 0, 0);
    AU4 const1 = AU4(0, 0, 0, 0);
    AU4 const2 = AU4(0, 0, 0, 0);
    AU4 const3 = AU4(0, 0, 0, 0);
	setup(const0, const1, const2, const3);
    
    // Do remapping of local xy in workgroup for a more PS-like swizzle pattern.
    AU2 pos = ARmp8x8(LocalThreadId.x) + AU2(WorkGroupId.x << 4u, WorkGroupId.y << 4u);
    

	filter(pos, const0, const1, const2, const3);

	pos.x += 8u;
	filter(pos, const0, const1, const2, const3);

	pos.y += 8u;
	filter(pos, const0, const1, const2, const3);

	pos.x -= 8u;
	filter(pos, const0, const1, const2, const3);
}