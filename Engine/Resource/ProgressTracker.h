#pragma once

#include <string>
#include <unordered_map>
#include "../EngineDefinition.h"

namespace PlayGround
{
	// ���α׷��� Ÿ��
	enum class EProgressType
	{
		ModelImporter,
		World,
		ResoruceCache
	};

	// ���α׷��� ����ü
	struct sProgress
	{
		sProgress()
		{
			Clear();
		}

		void Clear()
		{
			status.clear();
			jobs_done = 0;
			job_count = 0;
			is_loading = false;
		}

		// ���� ����
		std::string status;
		// �Ϸ�� �۾��� ����
		int jobs_done;
		// ��ü �۾��� ����
		int job_count;
		// �ε� ������
		bool is_loading;
	};

	class ProgressTracker
	{
	public:
		static ProgressTracker& Get()
		{
			static ProgressTracker instance;
			return instance;
		}

		ProgressTracker() = default;

		inline void Reset(EProgressType progress_type) { m_mapReports[progress_type].Clear(); }

		inline const std::string& GetStatus(EProgressType progress_type) { return m_mapReports[progress_type].status; }

		inline void SetStatus(EProgressType progress_type, const std::string& status) { m_mapReports[progress_type].status = status; }

		inline void SetJobCount(EProgressType progress_type, int job_count) { m_mapReports[progress_type].job_count = job_count; }

		inline void IncrementJobsDone(EProgressType progress_type) { m_mapReports[progress_type].jobs_done++; }

		inline void SetJobsDone(EProgressType progress_type, int jobs_done) { m_mapReports[progress_type].jobs_done = jobs_done; }

		inline float GetPercentage(EProgressType progress_type) { return m_mapReports[progress_type].job_count == 0 ? 0 : (static_cast<float>(m_mapReports[progress_type].jobs_done) / static_cast<float>(m_mapReports[progress_type].job_count)); }

		inline bool GetIsLoading(EProgressType progress_type) { return m_mapReports[progress_type].is_loading; }

		inline void SetIsLoading(EProgressType progress_type, bool is_loading) { m_mapReports[progress_type].is_loading = is_loading; }

	private:
		std::unordered_map<EProgressType, sProgress> m_mapReports;
	};
}