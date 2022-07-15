#include "Common.h"
#include "Threading.h"

using namespace std;

namespace PlayGround
{
	Threading::Threading(Context* context) : SubModule(context)
	{
		m_Stopping = false;
		// 최대 스레드 지원 갯수를 가져온다.
		m_ThreadCount_support = thread::hardware_concurrency();
		// 메인 스레드 제외
		m_ThreadCount = m_ThreadCount_support - 1;
		m_Thread_names[this_thread::get_id()] = "main";

		for (uint32_t i = 0; i < m_ThreadCount; i++)
		{
			m_vecThreads.emplace_back(thread(&Threading::ThreadLoop, this));
			m_Thread_names[m_vecThreads.back().get_id()] = "worker_" + to_string(i);
		}

		LOG_INFO("%d threads have been created", m_ThreadCount);
	}

	Threading::~Threading()
	{
		Flush(true);

		unique_lock<mutex> lock(m_Mutex_tasks);

		m_Stopping = true;

		lock.unlock();

		// 모든 스레드를 깨운다.
		m_Condition_var.notify_all();

		// 조인
		for (auto& thread : m_vecThreads)
		{
			thread.join();
		}

		// 스레드 초기화
		m_vecThreads.clear();
	}

	uint32_t Threading::GetThreadsAvailable() const
	{
		uint32_t available_threads = m_ThreadCount;

		for (const auto& task : m_Tasks)
		{
			available_threads -= task->IsExecuting() ? 1 : 0;
		}

		return available_threads;
	}

	void Threading::Flush(bool remove_queued /*= false*/)
	{
		// 태스크 클리어
		if (remove_queued)
			m_Tasks.clear();

		// 만약 아직 돌아가고 있는 스레드가 있을시에는 잠시 대기
		while (AreTasksRunning())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}

	// 스레드 루프
	void Threading::ThreadLoop()
	{
		shared_ptr<Task> task;

		while (true)
		{
			// 락
			unique_lock<mutex> lock(m_Mutex_tasks);

			// 스레드가 멈추려고 하거나 태스크가 없다면 기다린다.
			m_Condition_var.wait(lock, [this] {
				return !m_Tasks.empty() || m_Stopping;
			});

			// 스레드 루프 끝
			if (m_Stopping && m_Tasks.empty())
				return;

			// 태스크를 하나 꺼낸다.
			task = m_Tasks.front();

			m_Tasks.pop_front();

			lock.unlock();

			// 태스크 실행
			task->Execute();
		}
	}
}
