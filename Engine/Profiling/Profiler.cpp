#include "Common.h"
#include "Profiler.h"
#include "../Rendering/Renderer.h"
#include "../Resource/ResourceCache.h"
#include "../RHI/RHI_Device.h"
#include "../RHI/RHI_CommandList.h"
#include "../RHI/RHI_Implementation.h"
#include "../Core/EventSystem.h"

#include "../Core/Timer.h"


using namespace std;


namespace PlayGround
{
	static const int initial_capacity = 256;

	// 타임블록 최대치 초기화
	Profiler::Profiler(Context* context) : SubModule(context)
	{
		m_vecTime_blocks_read.reserve(initial_capacity);
		m_vecTime_blocks_read.resize(initial_capacity);
		m_vecTime_blocks_write.reserve(initial_capacity);
		m_vecTime_blocks_write.resize(initial_capacity);

		// 백버퍼 렌더링후의 이벤트 구족
		SUBSCRIBE_TO_EVENT(EventType::PostPresent, EVENT_HANDLER(OnPostPresent));
	}

	Profiler::~Profiler()
	{
		if (m_Poll)
			SwapBuffers();

		m_Renderer->GetRhiDevice()->QueryRelease(m_Query_disjoint);

		ClearRHIMetrics();
	}

	void Profiler::OnInit()
	{
		m_ResourceManager = m_Context->GetSubModule<ResourceCache>();
		m_Renderer = m_Context->GetSubModule<Renderer>();
		m_Timer = m_Context->GetSubModule<Timer>();
	}

	void Profiler::PrevUpdate()
	{
		RHI_Device* rhi_device = m_Renderer->GetRhiDevice().get();

		// 디바이스를 가져올수 없거나 디바이스 프로파일러를 가져올수 없다면 반환
		if (!rhi_device || !rhi_device->GetContextRhi()->profiler)
			return;

		// 쿼리 초기화
		if(m_Query_disjoint == nullptr)
			m_Renderer->GetRhiDevice()->QueryCreate(&m_Query_disjoint, RHI_Query_Type::Timestamp_Disjoint);

		// 용량 증가가 가능하다면
		if (m_Increase_capactiy)
		{
			SwapBuffers();

			// 사이즈는 두배로 늘린다.
			const uint32_t size_old = static_cast<uint32_t>(m_vecTime_blocks_write.size());
			const uint32_t size_new = size_old << 1;

			m_vecTime_blocks_read.reserve(size_new);
			m_vecTime_blocks_read.resize(size_new);
			m_vecTime_blocks_write.reserve(size_new);
			m_vecTime_blocks_write.resize(size_new);

			m_Increase_capactiy = false;
			m_Poll = true;

			LOG_WARNING("Time block list has grown to %d. Consider making the default capacity as large by default, to avoid re-allocating.", size_new);
		}

		ClearRHIMetrics();
	}

	void Profiler::PostUpdate()
	{
		// 타이밍 계산
		{
			// 버벅임 탐지
			float frames_to_accumulate = 5.0f;
			float delta_feedback = 1.0f / frames_to_accumulate;
			m_Is_stuttering_cpu = m_Time_cpu_last > (m_Time_cpu_avg + m_Stutter_delta_ms);
			m_Is_stuttering_gpu = m_Time_gpu_last > (m_Time_gpu_avg + m_Stutter_delta_ms);

			frames_to_accumulate = 20.0f;
			delta_feedback = 1.0f / frames_to_accumulate;
			m_Time_cpu_last = 0.0f;
			m_Time_gpu_last = 0.0f;

			// 타임블록들 순회
			for (const TimeBlock& time_block : m_vecTime_blocks_read)
			{
				if (!time_block.IsComplete())
					continue;

				// 시간을 누적한다.
				if (!time_block.GetParent() && time_block.GetType() == TimeBlockType::CPU)
					m_Time_cpu_last += time_block.GetDuration();

				if (!time_block.GetParent() && time_block.GetType() == TimeBlockType::GPU)
					m_Time_gpu_last += time_block.GetDuration();
			}
			
			
			// CPU 관련
			m_Time_cpu_avg = m_Time_cpu_avg * (1.0f - delta_feedback) + m_Time_cpu_last * delta_feedback;
			m_Time_cpu_min = Math::Util::Min(m_Time_cpu_min, m_Time_cpu_last);
			m_Time_cpu_max = Math::Util::Max(m_Time_cpu_max, m_Time_cpu_last);

			// GPU 관련
			m_Time_gpu_avg = m_Time_gpu_avg * (1.0f - delta_feedback) + m_Time_gpu_last * delta_feedback;
			m_Time_gpu_min = Math::Util::Min(m_Time_gpu_min, m_Time_gpu_last);
			m_Time_gpu_max = Math::Util::Max(m_Time_gpu_max, m_Time_gpu_last);

			// 프레임 관련
			m_Time_frame_last = static_cast<float>(m_Timer->GetDeltaTimeMs());
			m_Time_frame_avg = m_Time_frame_avg * (1.0f - delta_feedback) + m_Time_frame_last * delta_feedback;
			m_Time_frame_min = Math::Util::Min(m_Time_frame_min, m_Time_frame_last);
			m_Time_frame_max = Math::Util::Max(m_Time_frame_max, m_Time_frame_last);

			m_FPS = static_cast<float>(1.0f / m_Timer->GetDeltaTimeSec());
		}

		// 프로파일링 인터벌 계산후에 충분한 시간이 흘렀다면
		// 프로파일링을 갱신한다.
		m_Time_since_profiling_sec += static_cast<float>(m_Timer->GetDeltaTimeSec());

		if (m_Time_since_profiling_sec >= m_Profiling_interval_sec)
		{
			m_Time_since_profiling_sec = 0.0f;
			m_Poll = true;
		}
		else if (m_Poll)
			m_Poll = false;

		if (m_Poll)
		{
			m_Renderer->GetRhiDevice()->QueryBegin(m_Query_disjoint);

			AcquireGPUData();

			if (m_Renderer->GetOptions() & Renderer::Option::Debug_PerformanceMetrics)
			{
				UpdateRHIMetricsString();
			}
		}

		if (m_Profile && m_Poll)
			SwapBuffers();

	}

	void Profiler::OnPostPresent()
	{
		if (m_Poll)
		{
			m_Renderer->GetRhiDevice()->QueryEnd(m_Query_disjoint);
			m_Renderer->GetRhiDevice()->QueryGetData(m_Query_disjoint);
		}
	}

	// 버퍼 교체
	void Profiler::SwapBuffers()
	{
		{
			uint32_t pass_index_gpu = 0;

			for (uint32_t i = 0; i < static_cast<uint32_t>(m_vecTime_blocks_read.size()); i++)
			{
				TimeBlock& time_block = m_vecTime_blocks_write[i];

				// 모든 타임블록을 순회하면서 시간을 계산한다.
				if (time_block.IsComplete())
				{
					time_block.ComputeDuration(pass_index_gpu);

					if (time_block.GetType() == TimeBlockType::GPU)
						pass_index_gpu += 2;
				}
				else if (time_block.GetType() != TimeBlockType::Undefined)
				{
					LOG_WARNING("TimeBlockEnd() was not called for time block \"%s\"", time_block.GetName());
				}

				// 복사
				m_vecTime_blocks_read[i] = time_block;
				// 초기화
				m_vecTime_blocks_read[i].ClearGPUObjects();

				time_block.Reset();
			}
		}

		m_TimeBlock_index = -1;
	}


	void Profiler::TimeBlockStart(const char* func_name, TimeBlockType type, RHI_CommandList* cmd_list /*= nullptr*/)
	{
		if (!m_Profile || !m_Poll)
			return;

		// 프로파일링 여부 확인
		const bool can_profile_cpu = (type == TimeBlockType::CPU) && m_Profile_CPU;
		const bool can_profile_gpu = (type == TimeBlockType::GPU) && m_Profile_GPU;

		// 만약 cpu, gpu둘다 프로파일링 하지 않는다면 반환
		if (!can_profile_cpu && !can_profile_gpu)
			return;

		TimeBlock* time_block_parent = GetLastIncompleteTimeBlock(type);

		TimeBlock* time_block = GetNewTimeBlock();

		if (time_block)
			time_block->Begin(++m_Rhi_timeblock_count, func_name, type, time_block_parent, cmd_list);
	}

	void Profiler::TimeBlockEnd()
	{
		TimeBlock* time_block = GetLastIncompleteTimeBlock();

		if (time_block)
			time_block->End();
	}

	void Profiler::ResetMetrics()
	{
		m_Time_frame_avg = 0.0f;
		m_Time_frame_min = std::numeric_limits<float>::max();
		m_Time_frame_max = std::numeric_limits<float>::lowest();
		m_Time_frame_last = 0.0f;

		m_Time_cpu_avg = 0.0f;
		m_Time_cpu_min = std::numeric_limits<float>::max();
		m_Time_cpu_max = std::numeric_limits<float>::lowest();
		m_Time_cpu_last = 0.0f;

		m_Time_gpu_avg = 0.0f;
		m_Time_gpu_min = std::numeric_limits<float>::max();
		m_Time_gpu_max = std::numeric_limits<float>::lowest();
		m_Time_gpu_last = 0.0f;
	}

	TimeBlock* Profiler::GetNewTimeBlock()
	{
		// 허용치를 넘었다면 새롭게 할당
		if (m_TimeBlock_index + 1 >= static_cast<int>(m_vecTime_blocks_write.size()))
		{
			m_Increase_capactiy = true;
			return nullptr;
		}

		return &m_vecTime_blocks_write[++m_TimeBlock_index];
	}

	TimeBlock* Profiler::GetLastIncompleteTimeBlock(TimeBlockType type /*= TimeBlock_Undefined*/)
	{
		// 가장 최근에 완료되지 못한 타임블록을 가져온다.
		for (int i = m_TimeBlock_index; i >= 0; i--)
		{
			TimeBlock& time_block = m_vecTime_blocks_write[i];

			if (type == time_block.GetType() || type == TimeBlockType::Undefined)
			{
				if (!time_block.IsComplete())
					return &time_block;
			}
		}

		return nullptr;
	}

	void Profiler::AcquireGPUData()
	{
		RHI_Device* rhi_device = m_Renderer->GetRhiDevice().get();

		// 물리 디바이스 정보를 가져온다.
		const PhysicalDevice* physical_device = rhi_device->GetPrimaryPhysicalDevice();

		if (physical_device)
		{
			m_GPUName = physical_device->GetName();
			m_GPU_memory_used = RHI_CommandList::Gpu_GetMemoryUsed(rhi_device);
			m_GPU_memory_available = RHI_CommandList::Gpu_GetMemory(rhi_device);
			m_GPU_driver = physical_device->GetDriverVersion();
			m_GPU_api = rhi_device->GetContextRhi()->api_version;
		}
	}

	// 프로피일링 문자열
	void Profiler::UpdateRHIMetricsString()
	{
		const uint32_t texture_count = m_ResourceManager->GetResourceCount(EResourceType::Texture) + m_ResourceManager->GetResourceCount(EResourceType::Texture2d) + m_ResourceManager->GetResourceCount(EResourceType::TextureCube);
		const uint32_t material_count = m_ResourceManager->GetResourceCount(EResourceType::Material);

		static const char* text =
			// 시간
			"FPS:\t\t%.2f\n"
			"Frame:\t%d\n"
			"Time:\t%.2f ms\n"
			"\n"
			// 걸린 시간 통계
			"\t\tavg\t\tmin\t\tmax\t\tlast\n"
			"Total:\t%06.2f\t%06.2f\t%06.2f\t%06.2f ms\n"
			"CPU:\t%06.2f\t%06.2f\t%06.2f\t%06.2f ms\n"
			"GPU:\t%06.2f\t%06.2f\t%06.2f\t%06.2f ms\n"
			"\n"
			// GPU
			"GPU:\t%s\n"
			"VRAM:\t%d/%d MB\n"
			"API:\t\t%s\n"
			"Driver:\t%s\n"
			"\n"
			// 해상도
			"Output resolution:\t\t%dx%d\n"
			"Render resolution:\t%dx%d\n"
			"Viewport resolution:\t%dx%d\n"
			"\n"
			// 현재 렌더링되고있는 물체들 정보
			"Meshes rendered:\t%d\n"
			"Textures:\t\t%d\n"
			"Materials:\t\t%d\n"
			"\n"
			// RHI
			"Draw:\t\t\t\t\t%d\n"
			"Dispatch:\t\t\t\t%d\n"
			"Index buffer:\t\t\t\t%d\n"
			"Vertex buffer:\t\t\t%d\n"
			"Constant buffer:\t\t\t%d\n"
			"Sampler:\t\t\t\t\t%d\n"
			"Texture sampled:\t\t\t%d\n"
			"Texture storage:\t\t\t%d\n"
			"Shader vertex:\t\t\t%d\n"
			"Shader pixel:\t\t\t\t%d\n"
			"Shader compute:\t\t\t%d\n"
			"Render target:\t\t\t%d\n"
			"Pipeline:\t\t\t\t\t%d\n"
			"Descriptor set:\t\t\t%d\n"
			"Pipeline barrier:\t\t\t%d\n"
			"Descriptor pool capacity:\t%d";

		static char buffer[2048];
		sprintf_s
		(
			buffer, text,

			// 성능
			m_FPS,
			m_Renderer->GetFrameNum(),
			m_Time_frame_last,
			m_Time_frame_avg, m_Time_frame_min, m_Time_frame_max, m_Time_frame_last,
			m_Time_cpu_avg, m_Time_cpu_min, m_Time_cpu_max, m_Time_cpu_last,
			m_Time_gpu_avg, m_Time_gpu_min, m_Time_gpu_max, m_Time_gpu_last,

			m_GPUName.c_str(),
			m_GPU_memory_used, m_GPU_memory_available,
			m_GPU_api.c_str(),
			m_GPU_driver.c_str(),

			// 해상도
			static_cast<int>(m_Renderer->GetResolutionOutput().x), static_cast<int>(m_Renderer->GetResolutionOutput().y),
			static_cast<int>(m_Renderer->GetResolutionRender().x), static_cast<int>(m_Renderer->GetResolutionRender().y),
			static_cast<int>(m_Renderer->GetViewport().width), static_cast<int>(m_Renderer->GetViewport().height),

			// 렌더링
			m_Renderer_meshes_rendered,
			texture_count,
			material_count,

			// RHI
			m_Rhi_draw,
			m_Rhi_dispatch,
			m_Rhi_bindings_buffer_index,
			m_Rhi_bindings_buffer_vertex,
			m_Rhi_bindings_buffer_constant,
			m_Rhi_bindings_sampler,
			m_Rhi_bindings_texture_sampled,
			m_Rhi_bindings_texture_storage,
			m_Rhi_bindings_shader_vertex,
			m_Rhi_bindings_shader_pixel,
			m_Rhi_bindings_shader_compute,
			m_Rhi_bindings_render_target,
			m_Rhi_bindings_pipeline,
			m_Rhi_bindings_descriptor_set,
			m_Rhi_pipeline_barriers,
			m_descriptor_pool_capacity
		);

		m_Metrics = string(buffer);
	}
}