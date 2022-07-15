#pragma once

#include <unordered_map>
#include "IResource.h"
#include "../Core/SubModule.h"
#include "../Rendering/Model.h"


namespace PlayGround
{
	class FontImporter;
	class ImageImporter;
	class ModelImporter;

	// 리소스 경로
	enum class EResourceDirectory
	{
		CubeMaps,
		Fonts,
		Icons,
		ShaderCompiler,
		Shaders,
		Textures
	};

	// 리소스를 전체적으로 관리할 클래스
	class ResourceCache : public SubModule
	{
	public:
		ResourceCache(Context* context);
		~ResourceCache();

		// 서브 모듈 가상 메서드
		void OnInit() override;

		std::shared_ptr<IResource>& GetByName(const std::string& name, EResourceType type);

		template <typename T>
		constexpr std::shared_ptr<T> GetByName(const std::string& name) { return std::static_pointer_cast<T>(GetByName(name, IResource::TypeToEnum<T>())); }

		std::vector<std::shared_ptr<IResource>> GetByType(EResourceType type = EResourceType::Unknown);

		// T타입의 해당 경로를 가지고 있는 리소스를 반환한다.
		template <typename T>
		std::shared_ptr<T> GetByPath(const std::string& path)
		{
			for (std::shared_ptr<IResource>& resource : m_vecResources)
			{
				if (path == resource->GetResourceFilePathNative())
					return std::static_pointer_cast<T>(resource);
			}

			return nullptr;
		}

		// 리소스 캐싱
		template <typename T>
		std::shared_ptr<T> Cache(const std::shared_ptr<T>& resource)
		{
			if (!resource)
				return nullptr;

			// 엔진 자체 포맷이 아니고 유효한 경로가 아니라면 오류
			if (!resource->HasFilePathNative() && !FileSystem::IsDirectory(resource->GetResourceFilePathNative()))
			{
				LOG_ERROR("A resource must have a valid file path in order to be cached");
				return nullptr;
			}

			// 만약 자체 포맷이 아닐 경우도 캐싱 불가능
			if (!FileSystem::IsEngineFile(resource->GetResourceFilePathNative()))
			{
				LOG_ERROR("A resource must have a native file format in order to be cached, provide format was %s", FileSystem::GetExtensionFromFilePath(resource->GetResourceFilePathNative()).c_str());
				return nullptr;
			}

			// 이미 캐싱되어있는지 확인
			if (IsCached(resource->GetResourceName(), resource->GetResourceType()))
				return GetByName<T>(resource->GetResourceName());

			std::lock_guard<std::mutex> guard(m_Mutex);

			// 리소스 경로 설정
			resource->SaveToFile(resource->GetResourceFilePathNative());

			// 캐싱 성공
			return std::static_pointer_cast<T>(m_vecResources.emplace_back(resource));
		}

		template <typename T>
		std::shared_ptr<T> Load(const std::string& file_path)
		{
			// 경로 체크
			if (!FileSystem::Exists(file_path))
			{
				LOG_ERROR("\"%s\" doesn't exist.", file_path.c_str());
				return nullptr;
			}

			// 파일 이름
			const std::string name = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path);
			// 이미 캐싱 되어있는지 확인한다.
			if (IsCached(name, IResource::TypeToEnum<T>()))
				return GetByName<T>(name);

			auto typed = std::make_shared<T>(m_Context);

			// 리소스 경로 설정
			typed->SetResourceFilePath(file_path);

			// 만약 불러오지 못했다면 반환
			if (!typed || !typed->LoadFromFile(file_path))
			{
				LOG_ERROR("Failed to load \"%s\".", file_path.c_str());
				return nullptr;
			}

			// 캐싱
			return Cache<T>(typed);
		}

		// 리소스 삭제
		template <typename T>
		void Remove(std::shared_ptr<T>& resource)
		{
			if (!resource)
				return;

			// 만약 캐싱되어있는 리소스가 아니라면 반환
			if (!IsCached(resource->GetResourceName(), resource->GetResourceType()))
				return;

			// 저장되어있는 리소스에서 해당 리소스를 찾아서 삭제한다.
			m_vecResources.erase(std::remove_if(m_vecResources.begin(), m_vecResources.end(), [](std::shared_ptr<IResource> resource)
			{
				return dynamic_cast<EngineObject*>(resource.get())->GetObjectID() == resource->GetObjectID();
			}), m_vecResources.end());
		}

		uint64_t GetMemoryUsageCPU(EResourceType type = EResourceType::Unknown);
		uint64_t GetMemoryUsageGPU(EResourceType type = EResourceType::Unknown);
		uint32_t GetResourceCount(EResourceType type = EResourceType::Unknown);
		void Clear();

		void AddResourceDirectory(EResourceDirectory type, const std::string& directory);
		std::string GetResourceDirectory(EResourceDirectory type);
		void SetProjectDirectory(const std::string& directory);
		std::string GetProjectDirectoryAbsolute() const;
		inline const std::string& GetProjectDirectory() const {	return m_ProjectDirectory; }

		inline std::string GetResourceDirectory() const { return "Data"; }

		inline ModelImporter* GetModelImporter() const { return m_ModelImporter.get(); }

		inline ImageImporter* GetImageImporter() const { return m_ImageImporter.get(); }

		inline FontImporter* GetFontImporter() const { return m_FontImporter.get(); }

	private:
		bool IsCached(const uint64_t resource_id);
		bool IsCached(const std::string& resource_name, const EResourceType resource_type);

		void SaveResourcesToFiles();
		void LoadResourcesFromFiles();

		std::vector<std::shared_ptr<IResource>> m_vecResources;
		std::mutex m_Mutex;

		std::unordered_map<EResourceDirectory, std::string> m_mapStandard_resource_directories;
		std::string m_ProjectDirectory;

		std::shared_ptr<ModelImporter> m_ModelImporter;
		std::shared_ptr<ImageImporter> m_ImageImporter;
		std::shared_ptr<FontImporter> m_FontImporter;
	};
}

