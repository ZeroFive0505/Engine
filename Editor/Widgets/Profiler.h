#pragma once

#include "Widget.h"
#include "Profiling/Profiler.h"
#include "Math/MathUtil.h"
#include "Core/Timer.h"
#include <array>

// 프레임 최소, 평균, 최대치를 기록
struct sTimings
{
	sTimings()
	{
		Clear();
	}

	inline void AddSample(const float sample)
	{
		m_Min = PlayGround::Math::Util::Min(m_Min, sample);
		m_Max = PlayGround::Math::Util::Max(m_Max, sample);
		m_Sum += sample;
		m_SampleCount++;
		m_Avg = float(m_Sum / static_cast<float>(m_SampleCount));
	}

	inline void Clear()
	{
		m_Min = FLT_MAX;
		m_Max = FLT_MIN;
		m_Avg = 0.0f;
		m_Sum = 0.0f;
		m_SampleCount = 0;
	}

	float m_Min;
	float m_Max;
	float m_Avg;
	double m_Sum;
	uint64_t m_SampleCount;
};

// 프로파일러 위젯
class Profiler : public Widget
{
public:
	Profiler(Editor* editor);

	void OnShow() override;
	void OnHide() override;
	void UpdateVisible() override;

private:
	std::array<float, 400> m_Plot;
	sTimings m_Timings;
	PlayGround::Profiler* m_Profiler;
	int m_ItemType = 1;
};

