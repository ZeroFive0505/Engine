#pragma once

#include "Widget.h"
#include <memory>
#include <functional>
#include <deque>
#include <atomic>
#include "Log/ILogger.h"

// 로그 구조체
struct sLogPackage
{
	std::string text;
	unsigned int error_level = 0;
};

// 엔진의 로그를 상속받아 구현
class EngineLogger : public PlayGround::ILogger
{
public:
	// 콜백
	typedef std::function<void(sLogPackage)> log_func;

	// 콜백 설정
	inline void SetCallback(log_func&& func) { m_LogFunc = std::forward<log_func>(func); }

	// 로그 함수 오버라이드
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

// 에디터 콘솔 창
class Console : public Widget
{
public:
	Console(Editor* editor);

	void UpdateVisible() override;
	void AddLogPackage(const sLogPackage& package);
	void Clear();

private:
	// 제일 아래로 스크롤
	bool m_ScrollToBottom = false;
	// 로그 최대 갯수
	uint32_t m_LogMaxCount = 1000;
	// 로그가 보이는지 여부
	bool m_LogType_visibility[3] = { true, true, true };
	// 정보, 경고, 에러순으로 카운트한다.
	uint32_t m_LogType_counts[3] = { 0, 0, 0 };
	// 로그 색깔
	const std::vector<PlayGround::Math::Vector4> m_LogTypeColor =
	{
		PlayGround::Math::Vector4(0.76f, 0.77f, 0.8f, 1.0f),
		PlayGround::Math::Vector4(0.7f, 0.75f, 0.0f, 1.0f),
		PlayGround::Math::Vector4(0.7f, 0.3f, 0.3f, 1.0f)
	};
	// 스레드
	std::atomic<bool> m_IsReading = false;
	// 로거
	std::shared_ptr<EngineLogger> m_Logger;
	// 로그
	std::deque<sLogPackage> m_Logs;
	// 검색 필터
	ImGuiTextFilter m_LogFilter;
};

