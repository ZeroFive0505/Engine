#pragma once

#include <string>
#include "Widget.h"

// 리소스 로딩시 나오는 다이얼로그
class ProgressDialog : public Widget
{
public:
	ProgressDialog(Editor* editor);
	~ProgressDialog() = default;

	void UpdateAlways() override;
	void UpdateVisible() override;

private:
	float m_Progress;
	std::string m_ProgressStatus;
};

