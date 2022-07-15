#include "Common.hlsl"

#define A_GPU
#define A_HLSL
#define SPD_NO_WAVE_OPERATIONS
#define SPD_LINEAR_SAMPLER

#include "ffx_a.h"

groupshared AF4 spd_intermediate[16][16];
groupshared AU1 spd_counter;

AF4 SpdLoadSourceImage(ASU2 p, AU1 slice)
{
	float2 uv = (p + 0.5f) / g_resolution_rt;
	return tex.SampleLevel(sampler_bilinear_clamp, uv, 0);
}

// Load from mip 5
AF4 SpdLoad(ASU2 pos, AU1 slice)
{
	return tex_out_rgba_mips[5][pos];
}

void SpdStore(ASU2 pos, AF4 value, AU1 index, AU1 slice)
{
	tex_out_rgba_mips[index][pos] = value;
}

AF4 SpdLoadIntermediate(AU1 x, AU1 y)
{
	return spd_intermediate[x][y];
}

void SpdStoreIntermediate(AU1 x, AU1 y, AF4 value)
{
	spd_intermediate[x][y] = value;
}

AF4 SpdReduce4(AF4 s1, AF4 s2, AF4 s3, AF4 s4)
{
#if LUMINANCE_ANTIFLICKER
    // Karis's luma weighted average
    float s1w = 1 / (luminance(s1) + 1);
    float s2w = 1 / (luminance(s2) + 1);
    float s3w = 1 / (luminance(s3) + 1);
    float s4w = 1 / (luminance(s4) + 1);
    float one_div_wsum = 1.0 / (s1w + s2w + s3w + s4w);
    return (s1 * s1w + s2 * s2w + s3 * s3w + s4 * s4w) * one_div_wsum;
#else
	return (s1 + s2 + s3 + s4) * 0.25;
#endif
}

void SpdIncreaseAtomicCounter(AU1 slice)
{
	InterlockedAdd(g_atomic_counter[0], 1, spd_counter);
}

AU1 SpdGetAtomicCounter()
{
	return spd_counter;
}

void SpdResetAtomicCounter(AU1 slice)
{
	g_atomic_counter[0] = 0;
}

#include "ffx_spd.h"

[numthreads(256, 1, 1)]
void mainCS(uint3 work_group_id : SV_GroupID, uint local_thread_index : SV_GroupIndex)
{
	SpdDownsample(work_group_id.xy, local_thread_index, g_mip_count, g_work_group_count, work_group_id.z);
}
