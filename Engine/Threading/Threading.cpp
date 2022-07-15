#include "Common.h"
#include "Threading.h"

using namespace std;

namespace PlayGround
{
	Threading::Threading(Context* context) : SubModule(context)
	{
		m_Stopping = false;
		// �ִ� ������ ���� ������ �����´�.
		m_ThreadCount_support = thread::hardware_concurrency();
		// ���� ������ ����
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

		// ��� �����带 �����.
		m_Condition_var.notify_all();

		// ����
		for (auto& thread : m_vecThreads)
		{
			thread.join();
		}

		// ������ �ʱ�ȭ
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
		// �½�ũ Ŭ����
		if (remove_queued)
			m_Tasks.clear();

		// ���� ���� ���ư��� �ִ� �����尡 �����ÿ��� ��� ���
		while (AreTasksRunning())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}

	// ������ ����
	void Threading::ThreadLoop()
	{
		shared_ptr<Task> task;

		while (true)
		{
			// ��
			unique_lock<mutex> lock(m_Mutex_tasks);

			// �����尡 ���߷��� �ϰų� �½�ũ�� ���ٸ� ��ٸ���.
			m_Condition_var.wait(lock, [this] {
				return !m_Tasks.empty() || m_Stopping;
			});

			// ������ ���� ��
			if (m_Stopping && m_Tasks.empty())
				return;

			// �½�ũ�� �ϳ� ������.
			task = m_Tasks.front();

			m_Tasks.pop_front();

			lock.unlock();

			// �½�ũ ����
			task->Execute();
		}
	}
}
