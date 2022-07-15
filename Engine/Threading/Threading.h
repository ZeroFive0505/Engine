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
	// ������ Ǯ�� �߰��Ǵ� �½�ũ
	class Task
	{
	public:
		typedef std::function<void()> function_type;

		Task(function_type&& function)
		{
			// move semantic�� �̿�
			m_Function = std::forward<function_type>(function);
		}
		
		// �ݹ��Լ��� �����Ѵ�.
		void Execute()
		{
			// �۾� ��
			m_IsExecuting = true;
			// �ݹ� �Լ� ��
			m_Function();
			// �۾� ��
			m_IsExecuting = false;
		}

		inline bool IsExecuting() const { return m_IsExecuting; }

	private:
		bool m_IsExecuting = false;
		function_type m_Function;
	};

	// �������� ��ӹ޴� ������ Ŭ����
	class Threading : public SubModule
	{
	public:
		Threading(Context* context);
		~Threading();

		// Function�� �½�ũ�� �߰��Ѵ�.
		template <typename Function>
		void AddTask(Function&& function)
		{
			// ���� ����ִ� �����尡 ���ٸ�
			if (m_vecThreads.empty())
			{
				LOG_WARNING("No available threads, function will execute in the same thread");
				// �� �����忡�� �����Ѵ�.
				function();
				return;
			}

			// ��
			std::unique_lock<std::mutex> lock(m_Mutex_tasks);

			// �½�ũ�� �߰��Ѵ�.
			m_Tasks.push_back(std::make_shared<Task>(std::bind(std::forward<Function>(function))));

			// �� ����
			lock.unlock();

			// �����带 �����.
			m_Condition_var.notify_one();
		}

		// �ݺ��Ǵ� �½�ũ�� ������ �����忡 ���� �й�
		template <typename Function>
		void AddTaskLoop(Function&& function, uint32_t range)
		{
			// ���� ������ �������� ���� üũ�Ѵ�.
			uint32_t available_threads = GetThreadsAvailable();
			// �½�ũ �Ϸ� ����
			std::vector<bool> tasks_done = std::vector<bool>(available_threads, false);
			// ���� �������� ���� + 1
			const uint32_t task_count = available_threads + 1;

			uint32_t start = 0;
			uint32_t end = 0;

			for (uint32_t i = 0; i < available_threads; i++)
			{
				start = (range / task_count) * i;
				end = start + (range / task_count);

				// ������ ����
				AddTask([&function, &tasks_done, i, start, end] {
					function(start, end);
					tasks_done[i] = true;
				});
			}

			// ������ �۾��� �Ѵ�.
			function(end, range);

			uint32_t tasks = 0;

			// ��� �½�ũ�� ���������� ��ٸ���.
			while (tasks != tasks_done.size())
			{
				tasks = 0;

				// ������ ��� ī��Ʈ�� �ø���.
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

