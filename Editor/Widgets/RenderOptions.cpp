#include "RenderOptions.h"
#include "Rendering/Renderer.h"
#include "Core/Context.h"
#include "Core/Timer.h"
#include "Math/MathUtil.h"
#include "Rendering/Model.h"
#include "ImGuiHelper.h"
#include "../ImGui/Source/imgui_internal.h"
#include "RHI/RHI_Device.h"
#include "Profiling/Profiler.h"

using namespace std;
using namespace PlayGround;
using namespace PlayGround::Math;

// 렌더링 옵션 위젯에서 사용될 전역 변수 및 헬버 함수들
namespace Helper
{
	static Renderer* g_Renderer;

	static const float width_input_numeric = 120.0f;
	static const float width_combo_box = 120.0f;

	// 렌더러 초기화
	void Initialize(Renderer* renderer)
	{
		g_Renderer = renderer;
	}

	// 옵션
	bool Option(const char* title, bool default_open = true)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		return ImGuiEX::CollapsingHeader(title, default_open ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
	}

	// 첫번째 열
	void FirstColumn()
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
	}

	// 두번째 열
	void SecondColumn()
	{
		ImGui::TableSetColumnIndex(1);
	}

	// 체크박스
	bool CheckBox(const char* label, bool& option, const char* tooltip = nullptr)
	{
		// 첫번째 열에는 라벨과 툴팁이 있다면 툴팁
		FirstColumn();
		ImGui::Text(label);

		if (tooltip)
		{
			ImGuiEX::ToolTip(tooltip);
		}

		// 투번째 열에는 체크박스
		SecondColumn();
		ImGui::PushID(static_cast<int>(ImGui::GetCursorPosY()));
		ImGui::Checkbox("", &option);
		ImGui::PopID();

		return option;
	}

	// 콤보박스
	bool ComboBox(const char* label, const std::vector<std::string>& options, uint32_t& selection_index, const char* tooltip = nullptr)
	{
		// 첫번쩌 열에는 라벨과 툴팁
		FirstColumn();
		ImGui::Text(label);

		if (tooltip)
		{
			ImGuiEX::ToolTip(tooltip);
		}

		// 두번째 열에는 콤보박스
		SecondColumn();
		ImGui::PushID(static_cast<int>(ImGui::GetCursorPosY()));
		ImGui::PushItemWidth(width_combo_box);
		bool result = ImGuiEX::ComboBox("", options, &selection_index);
		ImGui::PopItemWidth();
		ImGui::PopID();

		return result;
	}

	// 렌더 옵션 값 설정
	void RenderOptionValue(const char* label, Renderer::OptionValue render_option, 
		const char* tooltip = nullptr, float step = 0.1f, float min = 0.0f, 
		float max = numeric_limits<float>::max(), const char* format = "%.3f")
	{
		// 첫번째 열에는 라벨과 툴팁
		FirstColumn();
		ImGui::Text(label);

		if (tooltip)
		{
			ImGuiEX::ToolTip(tooltip);
		}

		// 두번째 열에는 해당 렌더 옵션의 값
		SecondColumn();
		{
			float value = g_Renderer->GetOptionValue<float>(render_option);

			ImGui::PushID(static_cast<int>(ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(width_input_numeric);
			ImGui::InputFloat("", &value, step, 0.0f, format);
			ImGui::PopItemWidth();
			ImGui::PopID();
			value = Math::Util::Clamp(min, max, value);

			if (g_Renderer->GetOptionValue<float>(render_option) != value)
			{
				g_Renderer->SetOptionValue(render_option, value);
			}
		}
	}

	// 소수점 입력기
	void Float(const char* label, float& option, float step = 0.1f, const char* format = "%.3f")
	{
		FirstColumn();
		ImGui::Text(label);

		SecondColumn();
		{
			ImGui::PushID(static_cast<int>(ImGui::GetCursorPosY()));
			ImGui::PushItemWidth(width_input_numeric);
			ImGui::InputFloat("", &option, step, 0.0f, format);
			ImGui::PopItemWidth();
			ImGui::PopID();
		}
	}

	// 정수 입력기
	void Int(const char* label, int& option, int step = 1)
	{
		FirstColumn();
		ImGui::Text(label);
		SecondColumn();
		ImGui::PushID(static_cast<int>(ImGui::GetCursorPosY()));
		ImGui::PushItemWidth(width_input_numeric);
		ImGui::InputInt("##Shadow_resolution", &option, step);
		ImGui::PopItemWidth();
		ImGui::PopID();
	}
}

RenderOptions::RenderOptions(Editor* editor) : Widget(editor)
{
	// 렌더링 옵션 위젯 초기화

	m_Title = "Render Options";
	m_Flags |= ImGuiWindowFlags_NoDocking;
	m_Visible = false;
	m_Renderer = m_Context->GetSubModule<Renderer>();
	m_Alpha = 1.0f;
	m_Position = widget_position_screen_center;
	m_InitSize = Vector2(600.0f, 1000.0f);
}

void RenderOptions::UpdateVisible()
{
	// 렌더링 옵션 값 취득
	bool do_bloom = m_Renderer->GetOption(Renderer::Option::Bloom);
	bool do_dof = m_Renderer->GetOption(Renderer::Option::DepthOfField);
	bool do_volumetric_fog = m_Renderer->GetOption(Renderer::Option::VolumetricFog);
	bool do_ssao = m_Renderer->GetOption(Renderer::Option::Ssao);
	bool do_ssao_gi = m_Renderer->GetOption(Renderer::Option::Ssao_Gi);
	bool do_sss = m_Renderer->GetOption(Renderer::Option::ScreenSpaceShadows);
	bool do_ssr = m_Renderer->GetOption(Renderer::Option::ScreenSpaceReflections);
	bool do_taa = m_Renderer->GetOption(Renderer::Option::AntiAliasing_Taa);
	bool do_fxaa = m_Renderer->GetOption(Renderer::Option::AntiAliasing_Fxaa);
	bool do_motion_blur = m_Renderer->GetOption(Renderer::Option::MotionBlur);
	bool do_film_grain = m_Renderer->GetOption(Renderer::Option::FilmGrain);
	bool do_sharperning = m_Renderer->GetOption(Renderer::Option::Sharpening_AMD_FidelityFX_ContrastAdaptiveSharpening);
	bool do_chromatic_aberration = m_Renderer->GetOption(Renderer::Option::ChromaticAberration);
	bool do_debanding = m_Renderer->GetOption(Renderer::Option::Debanding);
	bool debug_physics = m_Renderer->GetOption(Renderer::Option::Debug_Physics);
	bool debug_aabb = m_Renderer->GetOption(Renderer::Option::Debug_Aabb);
	bool debug_light = m_Renderer->GetOption(Renderer::Option::Debug_Lights);
	bool debug_transform = m_Renderer->GetOption(Renderer::Option::Transform_Handle);
	bool debug_selection_outline = m_Renderer->GetOption(Renderer::Option::Debug_SelectionOutline);
	bool debug_picking_ray = m_Renderer->GetOption(Renderer::Option::Debug_PickingRay);
	bool debug_grid = m_Renderer->GetOption(Renderer::Option::Debug_Grid);
	bool debug_reflection_probes = m_Renderer->GetOption(Renderer::Option::Debug_ReflectionProbes);
	bool debug_performance_metrics = m_Renderer->GetOption(Renderer::Option::Debug_PerformanceMetrics);
	bool debug_wireframe = m_Renderer->GetOption(Renderer::Option::Debug_Wireframe);
	bool do_depth_prepass = m_Renderer->GetOption(Renderer::Option::DepthPrepass);
	bool do_reverse_z = m_Renderer->GetOption(Renderer::Option::ReverseZ);
	bool do_upsample_taa = m_Renderer->GetOption(Renderer::Option::Upsample_TAA);
	bool do_upsample_amd = m_Renderer->GetOption(Renderer::Option::Upsample_AMD_FidelityFX_SuperResolution);
	int resolution_shadow = m_Renderer->GetOptionValue<int>(Renderer::OptionValue::ShadowResolution);

	{
		static vector<sDisplayMode> display_modes;
		static vector<string> display_modes_string;
		const sDisplayMode& display_mode_active = Display::GetActiveDisplayMode();

		// 현재 디스플레이 모드가 비어있다면
		if (display_modes.empty())
		{
			// 모든 디스플레이 모드를 추가한다.
			for (const sDisplayMode& display_mode : Display::GetDisplayModes())
			{
				if (display_mode.hz == display_mode_active.hz)
				{
					display_modes.emplace_back(display_mode);
					display_modes_string.emplace_back(to_string(display_mode.width) + "x" + to_string(display_mode.height));
				}
			}
		}


		// 디스플레이 모드의 인덱스 반환 람다 함수
		const auto get_display_mode_index = [](const Vector2& resolution)
		{
			uint32_t index = 0;

			for (uint32_t i = 0; i < static_cast<uint32_t>(display_modes.size()); i++)
			{
				const sDisplayMode& display_mode = display_modes[i];

				if (display_mode.width == resolution.x && display_mode.height == resolution.y)
				{
					index = i;
					break;
				}
			}

			return index;
		};

		static ImVec2 size = ImVec2(0.0f);
		static int column_count = 2;
		static ImGuiTableFlags flags =
			ImGuiTableFlags_NoHostExtendX |   
			ImGuiTableFlags_BordersInnerV |   
			ImGuiTableFlags_SizingFixedFit;   

		Helper::Initialize(m_Renderer);

		// 렌더 옵션 테이블 시작
		if (ImGui::BeginTable("##Render_options", column_count, flags, size))
		{
			// 첫번째 열 옵션
			ImGui::TableSetupColumn("Option");
			// 두번째 열 옵션 값
			ImGui::TableSetupColumn("Value");
			ImGui::TableHeadersRow();

			// 해상도 옵션 시작
			if (Helper::Option("Resolution"))
			{
				Vector2 resolution_render = m_Renderer->GetResolutionRender();
				uint32_t resolution_render_index = get_display_mode_index(resolution_render);

				// 해상도 설정
				if (Helper::ComboBox("Render resolution", display_modes_string, resolution_render_index))
				{
					m_Renderer->SetResolutionRender(display_modes[resolution_render_index].width, display_modes[resolution_render_index].height);
				}

				// 출력 해상도 설정
				Vector2 resolution_output = m_Renderer->GetResolutionOutput();
				uint32_t resolution_output_index = get_display_mode_index(resolution_output);

				if (Helper::ComboBox("Output resolution", display_modes_string, resolution_output_index))
				{
					m_Renderer->SetResolutionOutput(display_modes[resolution_output_index].width, display_modes[resolution_output_index].height);
				}

				{
					// 만약 해상도가 출력 해상도보다 낮다면 업샘플링
					bool upsampling_allowed = resolution_render.x < resolution_output.x || resolution_render.y < resolution_output.y;

					// 업샘플링 모드
					static vector<string> upsampling_modes = { "Linear", "TAA upsampling - WIP", "AMD FidelityFX Super Resolution" };
					uint32_t upsampling_mode_index = do_upsample_taa ? 1 : (do_upsample_amd ? 2 : 0);

					// 업샘플링 기능 오프
					ImGui::BeginDisabled(!upsampling_allowed);

					// 기능이 오프됬을 시에는 업샘플링 설정 불가능
					if (Helper::ComboBox("Upsampling", upsampling_modes, upsampling_mode_index))
					{
						// 선형 모드
						if (upsampling_mode_index == 0)
						{
							do_upsample_taa = false;
							do_upsample_amd = false;
						}
						// TAA
						else if (upsampling_mode_index == 1)
						{
							do_upsample_taa = true;
							do_upsample_amd = false;
						}
						// AMD
						else if (upsampling_mode_index == 2)
						{
							do_upsample_taa = false;
							do_upsample_amd = true;
						}
					}

					ImGui::EndDisabled();
				}
			}

			// 스크린 스페이스 라이팅 옵션 시작
			if (Helper::Option("Screen space lighting"))
			{
				// SSR 설정
				Helper::CheckBox("SSR - Screen space reflections", do_ssr);

				// SSAO 설정
				Helper::CheckBox("SSAO - Screen space ambient occlusion", do_ssao);

				{
					// SSAO가 꺼져있다면
					ImGui::BeginDisabled(!do_ssao);
					// SSAO GI 설정 불가능
					Helper::CheckBox("SSAO GI - Screen space global illumination", do_ssao_gi, "Use SSAO to compute diffuse global illumination");
					ImGui::EndDisabled();
				}
			}

			// AA설정
			if (Helper::Option("Anti-Aliasing"))
			{
				// TAA
				Helper::CheckBox("TAA - Temporal anti-aliasing", do_taa, "Used to improve many stochastic effects, you want this to always be enabled.");

				// FXAA
				Helper::CheckBox("FXAA - Fast approximate anti-aliasing", do_fxaa);
			}

			// 카메라 옵션 시작
			if (Helper::Option("Camera"))
			{
				// 톤매핑 옵션
				static vector<string> tonemapping_options = { "Off", "ACES", "Reinhard", "Uncharted 2", "Matrix" };
				uint32_t selection_index = m_Renderer->GetOptionValue<uint32_t>(Renderer::OptionValue::Tonemapping);

				// 톤매핑 콤보박스
				if (Helper::ComboBox("Tonemapping", tonemapping_options, selection_index))
				{
					m_Renderer->SetOptionValue(Renderer::OptionValue::Tonemapping, static_cast<float>(selection_index));
				}

				// 감마 옵션
				Helper::RenderOptionValue("Gamma", Renderer::OptionValue::Gamma);

				// 블룸 체크 박스
				Helper::CheckBox("Bloom", do_bloom);
				{
					ImGui::BeginDisabled(!do_bloom);
					Helper::RenderOptionValue("Bloom intensity", Renderer::OptionValue::Bloom_Intensity, "", 0.001f);
					ImGui::EndDisabled();
				}

				// 모션 블러
				Helper::CheckBox("Motion blur (controlled by the camera's shutter speed)", do_motion_blur);

				// DOF
				Helper::CheckBox("Depth of field (controlled by the camera's aperture)", do_dof);

				// 색수차
				Helper::CheckBox("Chromatic aberration (controlled by the camera's aperture)", do_chromatic_aberration, "Emulates the inability of old cameras to focus all colors in the same focal point.");

				// Film grain
				Helper::CheckBox("Film grain", do_film_grain);
			}

			// 광원 옵션 시작
			if (Helper::Option("Lights"))
			{
				// 볼류메트릭 포그 체크박스
				Helper::CheckBox("Volumetric fog", do_volumetric_fog, "Requires a light with shadows enabled.");
				{
					ImGui::BeginDisabled(!do_volumetric_fog);
					Helper::RenderOptionValue("Volumetric fog density", Renderer::OptionValue::Fog, "", 0.01f, 0.0f, 16.0f, "%.2f");
					ImGui::EndDisabled();
				}

				// 스크린 스페이스 그림자 체크박스
				Helper::CheckBox("Screen space shadows", do_sss);

				// 그림자 해상도
				Helper::Int("Shadow resolution", resolution_shadow);
			}

			if (Helper::Option("Misc"))
			{
				Helper::CheckBox("Debanding", do_debanding, "Reduces color banding");

				Helper::CheckBox("Sharpening (AMD FidelityFX CAS)", do_sharperning, "Contrast adaptive sharpening. Areas of the image that are already sharp are sharpened less, while areas that lack detail are sharpened more.");

				ImGui::BeginDisabled(!do_sharperning);
				Helper::RenderOptionValue("Sharpening strength", Renderer::OptionValue::Sharpen_Strength, "", 0.1f, 0.0f, 1.0f);
				ImGui::EndDisabled();

				// 프레임 한계치 설정
				{
					Timer* timer = m_Context->GetSubModule<Timer>();

					Helper::FirstColumn();
					const FPSLimitType fps_limit_type = timer->GetFPSLimitType();
					string label = "FPS Limit - " + string((fps_limit_type == FPSLimitType::FixedToMonitor) ? 
						"Fixed to monitor" : (fps_limit_type == FPSLimitType::Unlocked ? "Unlocked" : "Fixed"));
					ImGui::Text(label.c_str());

					Helper::SecondColumn();
					{
						double fps_target = timer->GetFPSLimit();
						ImGui::PushItemWidth(Helper::width_input_numeric);
						ImGui::InputDouble("##FPS_limit", &fps_target, 0.0, 0.0, "%.1f");
						ImGui::PopItemWidth();
						timer->SetFPSLimit(fps_target);
					}
				}

				// 깊이 패스
				Helper::CheckBox("Depth PrePass", do_depth_prepass);
				// 리버스 Z
				Helper::CheckBox("Depth Reverse-Z", do_reverse_z);

				if (Helper::CheckBox("Performance Metrics", debug_performance_metrics) && !m_Renderer->GetOption(Renderer::Option::Debug_PerformanceMetrics))
				{
					m_Profiler->ResetMetrics();
				}
			}

			// 에디터 옵션
			if (Helper::Option("Editor", false))
			{
				// 트랜스폼 핸들러 사이즈 설정
				Helper::CheckBox("Transform", debug_transform);
				{
					ImGui::BeginDisabled(!debug_transform);
					Helper::Float("Transform size", m_Context->GetSubModule<World>()->m_gizmo_transform_size, 0.0025f);
					ImGui::EndDisabled();
				}

				// 여러 디버그 설정
				Helper::CheckBox("Selection outline", debug_selection_outline);
				Helper::CheckBox("Physics", debug_physics);
				Helper::CheckBox("AABBs - Axis-aligned bounding boxes", debug_aabb);
				Helper::CheckBox("Lights", debug_light);
				Helper::CheckBox("Picking ray", debug_picking_ray);
				Helper::CheckBox("Grid", debug_grid);
				Helper::CheckBox("Reflection probes", debug_reflection_probes);
				Helper::CheckBox("Wireframe", debug_wireframe);
			}

			ImGui::EndTable();
		}

		ImGui::PushItemWidth(m_Window->ContentSize.x - 60.0f);
		ImGui::SliderFloat("Opacity", &m_Alpha, 0.1f, 1.0f, "%.1f");
		ImGui::PopItemWidth();
	}

	m_Renderer->SetOption(Renderer::Option::Bloom, do_bloom);
	m_Renderer->SetOption(Renderer::Option::DepthOfField, do_dof);
	m_Renderer->SetOption(Renderer::Option::VolumetricFog, do_volumetric_fog);
	m_Renderer->SetOption(Renderer::Option::Ssao, do_ssao);
	m_Renderer->SetOption(Renderer::Option::Ssao_Gi, do_ssao_gi);
	m_Renderer->SetOption(Renderer::Option::ScreenSpaceShadows, do_sss);
	m_Renderer->SetOption(Renderer::Option::ScreenSpaceReflections, do_ssr);
	m_Renderer->SetOption(Renderer::Option::AntiAliasing_Taa, do_taa);
	m_Renderer->SetOption(Renderer::Option::AntiAliasing_Fxaa, do_fxaa);
	m_Renderer->SetOption(Renderer::Option::MotionBlur, do_motion_blur);
	m_Renderer->SetOption(Renderer::Option::FilmGrain, do_film_grain);
	m_Renderer->SetOption(Renderer::Option::Sharpening_AMD_FidelityFX_ContrastAdaptiveSharpening, do_sharperning);
	m_Renderer->SetOption(Renderer::Option::ChromaticAberration, do_chromatic_aberration);
	m_Renderer->SetOption(Renderer::Option::Debanding, do_debanding);
	m_Renderer->SetOption(Renderer::Option::Transform_Handle, debug_transform);
	m_Renderer->SetOption(Renderer::Option::Debug_SelectionOutline, debug_selection_outline);
	m_Renderer->SetOption(Renderer::Option::Debug_Physics, debug_physics);
	m_Renderer->SetOption(Renderer::Option::Debug_Aabb, debug_aabb);
	m_Renderer->SetOption(Renderer::Option::Debug_Lights, debug_light);
	m_Renderer->SetOption(Renderer::Option::Debug_PickingRay, debug_picking_ray);
	m_Renderer->SetOption(Renderer::Option::Debug_Grid, debug_grid);
	m_Renderer->SetOption(Renderer::Option::Debug_ReflectionProbes, debug_reflection_probes);
	m_Renderer->SetOption(Renderer::Option::Debug_PerformanceMetrics, debug_performance_metrics);
	m_Renderer->SetOption(Renderer::Option::Debug_Wireframe, debug_wireframe);
	m_Renderer->SetOption(Renderer::Option::DepthPrepass, do_depth_prepass);
	m_Renderer->SetOption(Renderer::Option::ReverseZ, do_reverse_z);
	m_Renderer->SetOption(Renderer::Option::Upsample_TAA, do_upsample_taa);
	m_Renderer->SetOption(Renderer::Option::Upsample_AMD_FidelityFX_SuperResolution, do_upsample_amd);
	m_Renderer->SetOptionValue(Renderer::OptionValue::ShadowResolution, static_cast<float>(resolution_shadow));
}