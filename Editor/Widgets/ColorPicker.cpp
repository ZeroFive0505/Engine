#include "ColorPicker.h"
#include "Rendering/Model.h"
#include "ImGuiHelper.h"

using namespace std;
using namespace PlayGround::Math;


// 칼라 피커 정보
namespace ColorPicker_Settings
{
	static string g_ButtonLabel;
	static string g_ColorPickerLabel;
	static bool showWheel = false;
	static bool showPreview = true;

	static bool hdr = false;
	static bool alpha_preview = true;
	static bool alpha_half_preview = false;
	static bool options_menu = true;
	static bool showRGB = true;
	static bool showHSV = false;
	static bool showHEX = true;
}

ColorPicker::ColorPicker(const string& windowTitle)
{
	// 칼라 피커 설정
	m_WindowTitle = windowTitle;
	ColorPicker_Settings::g_ButtonLabel = "##" + windowTitle + "1";
	ColorPicker_Settings::g_ColorPickerLabel = "##" + windowTitle + "1";

	m_Visible = false;
	m_Color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
}

void ColorPicker::Update()
{
	// 버튼 클릭시 화면에 보여준다.
	if (ImGui::ColorButton(ColorPicker_Settings::g_ButtonLabel.c_str(), m_Color))
		m_Visible = true;

	if (m_Visible)
		ShowColorPicker();
}

void ColorPicker::ShowColorPicker()
{
	// 칼라피커 시작
	ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin(m_WindowTitle.c_str(), &m_Visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking);
	ImGui::SetWindowFocus();

	// 플래그
	const int misc_flags = (ColorPicker_Settings::hdr ? ImGuiColorEditFlags_HDR : 0) |
		(ColorPicker_Settings::alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf :
			(ColorPicker_Settings::alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) |
		(ColorPicker_Settings::options_menu ? 0 : ImGuiColorEditFlags_NoOptions);

	ImGuiColorEditFlags flags = misc_flags;
	flags |= ImGuiColorEditFlags_AlphaBar;
	if (!ColorPicker_Settings::showPreview)    
		flags |= ImGuiColorEditFlags_NoSidePreview;

	flags |= ImGuiColorEditFlags_PickerHueBar;

	if (ColorPicker_Settings::showWheel)    
		flags |= ImGuiColorEditFlags_PickerHueWheel;

	if (ColorPicker_Settings::showRGB)   
		flags |= ImGuiColorEditFlags_DisplayRGB;

	if (ColorPicker_Settings::showHSV)       
		flags |= ImGuiColorEditFlags_DisplayHSV;

	if (ColorPicker_Settings::showHEX)     
		flags |= ImGuiColorEditFlags_DisplayHex;

	ImGui::ColorPicker4(ColorPicker_Settings::g_ColorPickerLabel.c_str(), (float*)&m_Color, flags);

	ImGui::Separator();

	ImGui::Text("Wheel");
	ImGui::SameLine();	ImGui::Checkbox("##ColorPickerWheel", &ColorPicker_Settings::showWheel);

	ImGui::SameLine();	ImGui::Text("RGB");
	ImGui::SameLine();	ImGui::Checkbox("##ColorPickerRGB", &ColorPicker_Settings::showRGB);

	ImGui::SameLine();	ImGui::Text("HSV");
	ImGui::SameLine();	ImGui::Checkbox("##ColorPickerHSV", &ColorPicker_Settings::showHSV);

	ImGui::SameLine();	ImGui::Text("HEX");
	ImGui::SameLine();	ImGui::Checkbox("##ColorPickerHEX", &ColorPicker_Settings::showHEX);

	ImGui::End();
}