#pragma once

#include "Widget.h"
#include <memory>
#include <functional>
#include <deque>
#include <atomic>
#include "Log/ILogger.h"

// �α� ����ü
struct sLogPackage
{
	std::string text;
	unsigned int error_level = 0;
};

// ������ �α׸� ��ӹ޾� ����
class EngineLogger : public PlayGround::ILogger
{
public:
	// �ݹ�
	typedef std::function<void(sLogPackage)> log_func;

	// �ݹ� ����
	inline void SetCallback(log_func&& func) { m_LogFunc = std::forward<log_func>(func); }

	// �α� �Լ� �������̵�
	void Log(const std::string& text, const unsigned int error_level) override
	{
		sLogPackage package;
		package.text = text;
		package.error_level = error_level;
		m_LogFunc(package);
	}

private:
	log_func m_LogFunc;
};

// ������ �ܼ� â
class Console : public Widget
{
public:
	Console(Editor* editor);

	void UpdateVisible() override;
	void AddLogPackage(const sLogPackage& package);
	void Clear();

private:
	// ���� �Ʒ��� ��ũ��
	bool m_ScrollToBottom = false;
	// �α� �ִ� ����
	uint32_t m_LogMaxCount = 1000;
	// �αװ� ���̴��� ����
	bool m_LogType_visibility[3] = { true, true, true };
	// ����, ���, ���������� ī��Ʈ�Ѵ�.
	uint32_t m_LogType_counts[3] = { 0, 0, 0 };
	// �α� ����
	const std::vector<PlayGround::Math::Vector4> m_LogTypeColor =
	{
		PlayGround::Math::Vector4(0.76f, 0.77f, 0.8f, 1.0f),
		PlayGround::Math::Vector4(0.7f, 0.75f, 0.0f, 1.0f),
		PlayGround::Math::Vector4(0.7f, 0.3f, 0.3f, 1.0f)
	};
	// ������
	std::atomic<bool> m_IsReading = false;
	// �ΰ�
	std::shared_ptr<EngineLogger> m_Logger;
	// �α�
	std::deque<sLogPackage> m_Logs;
	// �˻� ����
	ImGuiTextFilter m_LogFilter;
};

