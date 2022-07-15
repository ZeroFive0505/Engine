#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <unordered_map>
#include <functional>
#include "../Log/Logger.h"
#include "../Core/SubModule.h"


namespace PlayGround
{
	// 스레드 풀에 추가되는 태스크
	class Task
	{
	public:
		typedef std::function<void()> function_type;

		Task(function_type&& function)
		{
			// move semantic을 이용
			m_Function = std::forward<function_type>(function);
		}
		
		// 콜백함수를 실행한다.
		void Execute()
		{
			// 작업 중
			m_IsExecuting = true;
			// 콜백 함수 런
			m_Function();
			// 작업 끝
			m_IsExecuting = false;
		}

		inline bool IsExecuting() const { return m_IsExecuting; }

	private:
		bool m_IsExecuting = false;
		function_type m_Function;
	};

	// 서브모듈을 상속받는 스레드 클래스
	class Threading : public SubModule
	{
	public:
		Threading(Context* context);
		~Threading();

		// Function을 태스크에 추가한다.
		template <typename Function>
		void AddTask(Function&& function)
		{
			// 만약 비어있는 스레드가 없다면
			if (m_vecThreads.empty())
			{
				LOG_WARNING("No available threads, function will execute in the same thread");
				// 이 스레드에서 실행한다.
				function();
				return;
			}

			// 락
			std::unique_lock<std::mutex> lock(m_Mutex_tasks);

			// 태스크에 추가한다.
			m_Tasks.push_back(std::make_shared<Task>(std::bind(std::forward<Function>(function))));

			// 락 해제
			lock.unlock();

			// 스레드를 깨운다.
			m_Condition_var.notify_one();
		}

		// 반복되는 태스크를 가능한 스레드에 전부 분배
		template <typename Function>
		void AddTaskLoop(Function&& function, uint32_t range)
		{
			// 현재 가능한 스레드의 수를 체크한다.
			uint32_t available_threads = GetThreadsAvailable();
			// 태스크 완료 여부
			std::vector<bool> tasks_done = std::vector<bool>(available_threads, false);
			// 현재 스레드의 갯수 + 1
			const uint32_t task_count = available_threads + 1;

			uint32_t start = 0;
			uint32_t end = 0;

			for (uint32_t i = 0; i < available_threads; i++)
			{
				start = (range / task_count) * i;
				end = start + (range / task_count);

				// 스레드 시작
				AddTask([&function, &tasks_done, i, start, end] {
					function(start, end);
					tasks_done[i] = true;
				});
			}

			// 마지막 작업을 한다.
			function(end, range);

			uint32_t tasks = 0;

			// 모든 태스크가 끝날때까지 기다린다.
			while (tasks != tasks_done.size())
			{
				tasks = 0;

				// 끝났을 경우 카운트를 늘린다.
				for (const bool job_done : tasks_done)
				{
					tasks += job_done ? 1 : 0;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}
		}

		inline uint32_t GetThreadCount() const { return m_ThreadCount; }

		inline uint32_t GetThreadCountSupport() const { return m_ThreadCount_support; }

		uint32_t GetThreadsAvailable() const;

		inline bool AreTasksRunning() const { return GetThreadsAvailable() != GetThreadCount(); }

		void Flush(bool removed_queue = false);

	private:
		void ThreadLoop();

		uint32_t m_ThreadCount = 0;
		uint32_t m_ThreadCount_support = 0;
		std::vector<std::thread> m_vecThreads;
		std::deque<std::shared_ptr<Task>> m_Tasks;
		std::mutex m_Mutex_tasks;
		std::condition_variable m_Condition_var;
		std::unordered_map<std::thread::id, std::string> m_Thread_names;
		bool m_Stopping;
	};
}

