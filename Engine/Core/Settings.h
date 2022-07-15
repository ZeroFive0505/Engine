#pragma once

#include "SubModule.h"
#include "../Math/Vector2.h"
#include <vector>

namespace PlayGround
{
	class Context;
	
	// 외부 라이브러리를 정보를 위한 구조체
	struct sThirdParty
	{
		sThirdParty(const std::string& _name, const std::string& _version, const std::string& _url)
		{
			name = _name;
			version = _version;
			url = _url;
		}

		// 이름, 버전, 주소
		std::string name;
		std::string version;
		std::string url;
	};

	// 엔진의 설정을 저장하고 불러오는 클래스
	class Settings : public SubModule
	{
	public:
		Settings(Context* context);
		~Settings();

		void OnPostInit() override;
		void OnExit() override;

		void RegisterThirdParty(const std::string& name, const std::string& version, const std::string& url);

		inline const std::vector<sThirdParty>& GetThirdParties() const
		{
			return m_vecThirdparties;
		}

		inline bool GetIsFullScreen() const
		{
			return m_IsFullScreen;
		}

		inline bool GetIsMouseVisible() const
		{
			return m_IsMouseVisible;
		}

		inline bool HasLoadedUserSettings() const
		{
			return m_HasLoaded_user_settings;
		}

		inline const Math::Vector2& GetResolutionOutput() const
		{
			return m_Resolution_output;
		}

	private:
		void Save() const;
		void Load();

		void Map() const;
		void Reflect();

		bool m_IsFullScreen = false;
		bool m_IsMouseVisible = true;
		uint32_t m_ShadowMap_resolution = 0;
		uint64_t m_Renderer_flags = 0;
		Math::Vector2 m_Resolution_output = Math::Vector2::Zero;
		Math::Vector2 m_Resolution_render = Math::Vector2::Zero;
		uint32_t m_Anisotropy = 0;
		uint32_t m_Tonemapping = 0;
		uint32_t m_MaxThreadCount = 0;
		double m_FpsLimit = 0;
		bool m_HasLoaded_user_settings = false;
		Context* m_Context = nullptr;
		std::vector<sThirdParty> m_vecThirdparties;
	};
}