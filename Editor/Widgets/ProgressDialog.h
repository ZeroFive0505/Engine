#pragma once

#include <string>
#include "Widget.h"

// ���ҽ� �ε��� ������ ���̾�α�
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

