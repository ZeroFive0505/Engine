#pragma once

#include <string>
#include <vector>
#include "TimeBlock.h"
#include "../Core/SubModule.h"
#include "../Core/StopWatch.h"
#include "../EngineDefinition.h"


// 프로파일링 매크로
#define TIME_BLOCK_START_NAMED(profiler, name) profiler->TimeBlockStart(name, PlayGround::TimeBlockType::CPU, nullptr);
#define TIME_BLOCK_END(profiler) profiler->TimeBlockEnd();
#define SCOPED_TIME_BLOCK(profiler) if(profiler) { ScopedTimeBlock time_block = ScopedTimeBlock(profiler, __FUNCTION__); }

namespace PlayGround
{
	class Context;
	class Timer;
	class ResourceCache;
	class Renderer;
	class Variant;
	class Timer;

	// 프로파일러, 서브모듈을 상속받는다.
	class Profiler : public SubModule
	{
	public:
		Profiler(Context* context);
		~Profiler();

		// 서브 모듈 가상함수 오버라이드
		void OnInit() override;
		void PrevUpdate() override;
		void PostUpdate() override;

		void TimeBlockStart(const char* func_name, TimeBlockType type, RHI_CommandList* cmd_list = nullptr);
		void TimeBlockEnd();
		void ResetMetrics();

		// 프로파일링 여부
		inline bool GetEnabled() const { return m_Profile; }

		// 프로파일링 온 오프
		inline void SetEnabled(const bool enabled) { m_Profile = enabled; }

		// 프레임 등 프로파일링 문자열 반환
		inline const std::string& GetMetrics() const { return m_Metrics; }

		// 타임블록들 반환
		inline const std::vector<TimeBlock>& GetTimeBlocks() const { return m_vecTime_blocks_read; }

		// 마지막 CPU타임을 반환
		inline float GetTimeCPULast() const { return m_Time_cpu_last; }

		// 마지막 GPU타임을 반환
		inline float GetTimeGPULast() const { return m_Time_gpu_last; }

		// 마지막 프레임을 반환
		inline float GetTimeFrameLast() const { return m_Time_frame_last; }

		// FPS를 반환
		inline float GetFPS() const { return m_FPS; }

		// 업데이트 인터벌 반환
		inline float GetUpdateInterval() const { return m_Profiling_interval_sec; }

		inline void SetUpdateInverval(float interval)
		{
			if (interval < 0.0f)
				return;

			m_Profiling_interval_sec = interval;
		}

		// GPU 이름
		inline const std::string& GetGpuName() const { return m_GPUName; }

		// GPU 메모리 크기
		inline uint32_t GetGpuMemoryAvailable() const { return m_GPU_memory_available; }

		// GPU 메모리 사용량
		inline uint32_t GetGpuMemoryUsed() const { return m_GPU_memory_used; }

		// CPU 버벅임
		inline bool IsCPUStuttering() const { return m_Is_stuttering_cpu; }

		// GPU 버벅임
		inline bool IsGPUStuttering() const { return m_Is_stuttering_gpu; }

		// 렌더링시 드로우콜의 횟수, 바인딩되는 리소스들의 수
		uint32_t m_Rhi_draw = 0;
		uint32_t m_Rhi_dispatch = 0;
		uint32_t m_Rhi_bindings_buffer_index = 0;
		uint32_t m_Rhi_bindings_buffer_vertex = 0;
		uint32_t m_Rhi_bindings_buffer_constant = 0;
		uint32_t m_Rhi_bindings_buffer_structured = 0;
		uint32_t m_Rhi_bindings_sampler = 0;
		uint32_t m_Rhi_bindings_texture_sampled = 0;
		uint32_t m_Rhi_bindings_shader_vertex = 0;
		uint32_t m_Rhi_bindings_shader_pixel = 0;
		uint32_t m_Rhi_bindings_shader_compute = 0;
		uint32_t m_Rhi_bindings_render_target = 0;
		uint32_t m_Rhi_bindings_texture_storage = 0;
		uint32_t m_Rhi_bindings_descriptor_set = 0;
		uint32_t m_Rhi_bindings_pipeline = 0;
		uint32_t m_Rhi_pipeline_barriers = 0;
		uint32_t m_Rhi_timeblock_count = 0;

		uint32_t m_Renderer_meshes_rendered = 0;

		float m_Time_frame_avg = 0.0f;
		float m_Time_frame_min = std::numeric_limits<float>::max();
		float m_Time_frame_max = std::numeric_limits<float>::lowest();
		float m_Time_frame_last = 0.0f;
		float m_Time_cpu_avg = 0.0f;
		float m_Time_cpu_min = std::numeric_limits<float>::max();
		float m_Time_cpu_max = std::numeric_limits<float>::lowest();
		float m_Time_cpu_last = 0.0f;
		float m_Time_gpu_avg = 0.0f;
		float m_Time_gpu_min = std::numeric_limits<float>::max();
		float m_Time_gpu_max = std::numeric_limits<float>::lowest();
		float m_Time_gpu_last = 0.0f;

		uint32_t m_descriptor_pool_capacity = 0;
	private:
		// 백퍼버 렌더 후
		void OnPostPresent();

		// 버퍼 스왑
		void SwapBuffers();

		// 기록 변수 초기화
		void ClearRHIMetrics()
		{
			m_Rhi_draw = 0;
			m_Rhi_dispatch = 0;
			m_Renderer_meshes_rendered = 0;
			m_Rhi_bindings_buffer_index = 0;
			m_Rhi_bindings_buffer_vertex = 0;
			m_Rhi_bindings_buffer_constant = 0;
			m_Rhi_bindings_buffer_structured = 0;
			m_Rhi_bindings_sampler = 0;
			m_Rhi_bindings_texture_sampled = 0;
			m_Rhi_bindings_shader_vertex = 0;
			m_Rhi_bindings_shader_pixel = 0;
			m_Rhi_bindings_shader_compute = 0;
			m_Rhi_bindings_render_target = 0;
			m_Rhi_bindings_texture_storage = 0;
			m_Rhi_bindings_descriptor_set = 0;
			m_Rhi_bindings_pipeline = 0;
			m_Rhi_pipeline_barriers = 0;
			m_Rhi_timeblock_count = 0;
		}

		// 타임블록 가져오기
		TimeBlock* GetNewTimeBlock();
		// GPU 데이터 가져오기
		void AcquireGPUData();
		// 기록 업데이트
		void UpdateRHIMetricsString();
		// 최근에 완성되지 못한 타임블록 가져오기
		TimeBlock* GetLastIncompleteTimeBlock(TimeBlockType type = TimeBlockType::Undefined);

		bool m_Profile = false;
		bool m_Profile_CPU = true;
		bool m_Profile_GPU = true;
		float m_Profiling_interval_sec = 0.2f;
		float m_Time_since_profiling_sec = m_Profiling_interval_sec;

		int m_TimeBlock_index = -1;
		std::vector<TimeBlock> m_vecTime_blocks_write;
		std::vector<TimeBlock> m_vecTime_blocks_read;

		float m_FPS = 0.0f;

		std::string m_GPUName = "N/A";
		std::string m_GPU_driver = "N/A";
		std::string m_GPU_api = "N/A";
		uint32_t m_GPU_memory_available = 0;
		uint32_t m_GPU_memory_used = 0;

		float m_Stutter_delta_ms = 0.5f;
		bool m_Is_stuttering_cpu = false;
		bool m_Is_stuttering_gpu = false;

		bool m_Poll = false;
		std::string m_Metrics = "N/A";
		bool m_Increase_capactiy = false;
		bool m_AllowTime_block_end = true;
		void* m_Query_disjoint = nullptr;

		ResourceCache* m_ResourceManager = nullptr;
		Renderer* m_Renderer = nullptr;
		Timer* m_Timer = nullptr;
	};

	class ScopedTimeBlock
	{
	public:
		// 생성자에는 타임블록 스타트
		ScopedTimeBlock(Profiler* _profiler, const char* name = nullptr)
		{
			profiler = _profiler;
			profiler->TimeBlockStart(name, PlayGround::TimeBlockType::CPU);
		}

		// 소멸자에서는 타임블록 엔드
		~ScopedTimeBlock()
		{
			profiler->TimeBlockEnd();
		}

	private:
		Profiler* profiler = nullptr;
	};
}
