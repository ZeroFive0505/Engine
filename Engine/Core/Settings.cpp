#include "Common.h"
#include "Settings.h"
#include "Window.h"
#include "../Core/FileSystem.h"
#include "../Rendering/Renderer.h"
#include "../Threading/Threading.h"
#include "../Display/Display.h"
#include "../Input/Input.h"
#include "Context.h"
#include "Timer.h"

using namespace std;
using namespace PlayGround::Math;


namespace EngineSettings
{
	ofstream fout;
	ifstream fin;
	// 엔진 설정 파일의 이름
	string file_name = "engine.ini";

	// 설정 값을 이름 = 값 이런식으로 쓴다.
	template <typename T>
	void write_setting(ofstream& fout, const string& name, T value)
	{
		fout << name << "=" << value << endl;
	}

	// 설정 값을 불러온다.
	template <typename T>
	void read_setting(ifstream& fin, const string& name, T& value)
	{
		for (string line; getline(fin, line);)
		{
			// =를 찾는다.
			const auto first_index = line.find_first_of('=');

			// =의 위치를 기준으로 이름과 값을 찾아온다.
			if (name == line.substr(0, first_index))
			{
				const auto lastIndex = line.find_last_of('=');
				const auto read_value = line.substr(lastIndex + 1, line.length());
				value = static_cast<T>(stof(read_value));
				return;
			}
		}
	}
}

namespace PlayGround
{
	Settings::Settings(Context* context) : SubModule(context)
	{
		m_Context = context;

		m_Resolution_output.x = static_cast<float>(Display::GetWidth());
		m_Resolution_output.y = static_cast<float>(Display::GetHeight());
		m_Resolution_render = m_Resolution_output;

		RegisterThirdParty("pugixml", "1.11.4", "https://github.com/zeux/pugixml");
		RegisterThirdParty("DirectXShaderCompiler", "1.6.2109", "https://github.com/microsoft/DirectXShaderCompiler");
	}

	Settings::~Settings()
	{

	}

	void Settings::OnPostInit()
	{
		// 설정을 반영한다.
		Reflect();

		// 만약 설정 파일이 존재한다면 불러온다.
		if (FileSystem::Exists(EngineSettings::file_name))
		{
			Load();
			Map();
		}
		else
			Save();

		// 설정 값 로그
		LOG_INFO("Resolution: %dx%d.", static_cast<int>(m_Resolution_render.x), static_cast<int>(m_Resolution_render.y));
		LOG_INFO("FPS Limit: %f.", m_FpsLimit);
		LOG_INFO("Shadow resolution: %d.", m_ShadowMap_resolution);
		LOG_INFO("Anisotropy: %d.", m_Anisotropy);
		LOG_INFO("Max threads: %d.", m_MaxThreadCount);
	}

	void Settings::OnExit()
	{
		Reflect();
		Save();
	}

	void Settings::RegisterThirdParty(const std::string& name, const std::string& version, const std::string& url)
	{
		m_vecThirdparties.emplace_back(name, version, url);
	}

	void Settings::Save() const
	{
		EngineSettings::fout.open(EngineSettings::file_name, ofstream::out);

		EngineSettings::write_setting(EngineSettings::fout, "bFullScreen", m_IsFullScreen);
		EngineSettings::write_setting(EngineSettings::fout, "bIsMouseVisible", m_IsMouseVisible);
		EngineSettings::write_setting(EngineSettings::fout, "iResolutionOutputWidth", m_Resolution_output.x);
		EngineSettings::write_setting(EngineSettings::fout, "iResolutionOutputHeight", m_Resolution_output.y);
		EngineSettings::write_setting(EngineSettings::fout, "iResolutionRenderWidth", m_Resolution_render.x);
		EngineSettings::write_setting(EngineSettings::fout, "iResolutionRenderHeight", m_Resolution_render.y);
		EngineSettings::write_setting(EngineSettings::fout, "iShadowMapResolution", m_ShadowMap_resolution);
		EngineSettings::write_setting(EngineSettings::fout, "iAnisotropy", m_Anisotropy);
		EngineSettings::write_setting(EngineSettings::fout, "iTonemapping", m_Tonemapping);
		EngineSettings::write_setting(EngineSettings::fout, "fFPSLimit", m_FpsLimit);
		EngineSettings::write_setting(EngineSettings::fout, "iMaxThreadCount", m_MaxThreadCount);
		EngineSettings::write_setting(EngineSettings::fout, "iRendererFlags", m_Renderer_flags);

		EngineSettings::fout.close();
	}

	void Settings::Load()
	{
		EngineSettings::fin.open(EngineSettings::file_name, ifstream::in);

		EngineSettings::read_setting(EngineSettings::fin, "bFullScreen", m_IsFullScreen);
		EngineSettings::read_setting(EngineSettings::fin, "bIsMouseVisible", m_IsMouseVisible);
		EngineSettings::read_setting(EngineSettings::fin, "iResolutionOutputWidth", m_Resolution_output.x);
		EngineSettings::read_setting(EngineSettings::fin, "iResolutionOutputHeight", m_Resolution_output.y);
		EngineSettings::read_setting(EngineSettings::fin, "iResolutionRenderWidth", m_Resolution_render.x);
		EngineSettings::read_setting(EngineSettings::fin, "iResolutionRenderHeight", m_Resolution_render.y);
		EngineSettings::read_setting(EngineSettings::fin, "iShadowMapResolution", m_ShadowMap_resolution);
		EngineSettings::read_setting(EngineSettings::fin, "iAnisotropy", m_Anisotropy);
		EngineSettings::read_setting(EngineSettings::fin, "iTonemapping", m_Tonemapping);
		EngineSettings::read_setting(EngineSettings::fin, "fFPSLimit", m_FpsLimit);
		EngineSettings::read_setting(EngineSettings::fin, "iMaxThreadCount", m_MaxThreadCount);
		EngineSettings::read_setting(EngineSettings::fin, "iRendererFlags", m_Renderer_flags);

		EngineSettings::fin.close();

		m_HasLoaded_user_settings = true;
	}

	void Settings::Map() const
	{
		// 불러온 설정값으로 타이머를 설정
		Timer* timer = m_Context->GetSubModule<Timer>();
		
		if (timer)
			timer->SetFPSLimit(m_FpsLimit);

		// 입력 클래스도 설정
		Input* input = m_Context->GetSubModule<Input>();

		if (input)
			input->SetMouseCursorVisible(m_IsMouseVisible);

		Renderer* renderer = m_Context->GetSubModule<Renderer>();

		// 렌더러의 설정
		if (renderer)
		{
			renderer->SetResolutionOutput(static_cast<uint32_t>(m_Resolution_output.x), static_cast<uint32_t>(m_Resolution_output.y));
			renderer->SetResolutionRender(static_cast<uint32_t>(m_Resolution_render.x), static_cast<uint32_t>(m_Resolution_render.y));
			renderer->SetOptionValue(Renderer::OptionValue::ShadowResolution, static_cast<float>(m_ShadowMap_resolution));
			renderer->SetOptionValue(Renderer::OptionValue::Anisotropy, static_cast<float>(m_Anisotropy));
			renderer->SetOptionValue(Renderer::OptionValue::Tonemapping, static_cast<float>(m_Tonemapping));
			renderer->SetOptions(m_Renderer_flags);
		}

		Window* window = m_Context->GetSubModule<Window>();

		if (window)
		{
			if (m_IsFullScreen)
			{
				window->FullScreen();
			}
		}
	}

	void Settings::Reflect()
	{
		Renderer* renderer = m_Context->GetSubModule<Renderer>();

		m_FpsLimit = m_Context->GetSubModule<Timer>()->GetFPSLimit();
		m_MaxThreadCount = m_Context->GetSubModule<Threading>()->GetThreadCountSupport();
		m_IsFullScreen = m_Context->GetSubModule<Window>()->IsFullScreen();
		m_IsMouseVisible = m_Context->GetSubModule<Input>()->GetMouseCursorVisible();
		m_Resolution_output = renderer->GetResolutionOutput();
		m_Resolution_render = renderer->GetResolutionRender();
		m_ShadowMap_resolution = renderer->GetOptionValue<uint32_t>(Renderer::OptionValue::ShadowResolution);
		m_Anisotropy = renderer->GetOptionValue<uint32_t>(Renderer::OptionValue::Anisotropy);
		m_Tonemapping = renderer->GetOptionValue<uint32_t>(Renderer::OptionValue::Tonemapping);
		m_Renderer_flags = renderer->GetOptions();
	}
}
