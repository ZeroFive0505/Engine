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

	// ���ҽ� ���
	enum class EResourceDirectory
	{
		CubeMaps,
		Fonts,
		Icons,
		ShaderCompiler,
		Shaders,
		Textures
	};

	// ���ҽ��� ��ü������ ������ Ŭ����
	class ResourceCache : public SubModule
	{
	public:
		ResourceCache(Context* context);
		~ResourceCache();

		// ���� ��� ���� �޼���
		void OnInit() override;

		std::shared_ptr<IResource>& GetByName(const std::string& name, EResourceType type);

		template <typename T>
		constexpr std::shared_ptr<T> GetByName(const std::string& name) { return std::static_pointer_cast<T>(GetByName(name, IResource::TypeToEnum<T>())); }

		std::vector<std::shared_ptr<IResource>> GetByType(EResourceType type = EResourceType::Unknown);

		// TŸ���� �ش� ��θ� ������ �ִ� ���ҽ��� ��ȯ�Ѵ�.
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

		// ���ҽ� ĳ��
		template <typename T>
		std::shared_ptr<T> Cache(const std::shared_ptr<T>& resource)
		{
			if (!resource)
				return nullptr;

			// ���� ��ü ������ �ƴϰ� ��ȿ�� ��ΰ� �ƴ϶�� ����
			if (!resource->HasFilePathNative() && !FileSystem::IsDirectory(resource->GetResourceFilePathNative()))
			{
				LOG_ERROR("A resource must have a valid file path in order to be cached");
				return nullptr;
			}

			// ���� ��ü ������ �ƴ� ��쵵 ĳ�� �Ұ���
			if (!FileSystem::IsEngineFile(resource->GetResourceFilePathNative()))
			{
				LOG_ERROR("A resource must have a native file format in order to be cached, provide format was %s", FileSystem::GetExtensionFromFilePath(resource->GetResourceFilePathNative()).c_str());
				return nullptr;
			}

			// �̹� ĳ�̵Ǿ��ִ��� Ȯ��
			if (IsCached(resource->GetResourceName(), resource->GetResourceType()))
				return GetByName<T>(resource->GetResourceName());

			std::lock_guard<std::mutex> guard(m_Mutex);

			// ���ҽ� ��� ����
			resource->SaveToFile(resource->GetResourceFilePathNative());

			// ĳ�� ����
			return std::static_pointer_cast<T>(m_vecResources.emplace_back(resource));
		}

		template <typename T>
		std::shared_ptr<T> Load(const std::string& file_path)
		{
			// ��� üũ
			if (!FileSystem::Exists(file_path))
			{
				LOG_ERROR("\"%s\" doesn't exist.", file_path.c_str());
				return nullptr;
			}

			// ���� �̸�
			const std::string name = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path);
			// �̹� ĳ�� �Ǿ��ִ��� Ȯ���Ѵ�.
			if (IsCached(name, IResource::TypeToEnum<T>()))
				return GetByName<T>(name);

			auto typed = std::make_shared<T>(m_Context);

			// ���ҽ� ��� ����
			typed->SetResourceFilePath(file_path);

			// ���� �ҷ����� ���ߴٸ� ��ȯ
			if (!typed || !typed->LoadFromFile(file_path))
			{
				LOG_ERROR("Failed to load \"%s\".", file_path.c_str());
				return nullptr;
			}

			// ĳ��
			return Cache<T>(typed);
		}

		// ���ҽ� ����
		template <typename T>
		void Remove(std::shared_ptr<T>& resource)
		{
			if (!resource)
				return;

			// ���� ĳ�̵Ǿ��ִ� ���ҽ��� �ƴ϶�� ��ȯ
			if (!IsCached(resource->GetResourceName(), resource->GetResourceType()))
				return;

			// ����Ǿ��ִ� ���ҽ����� �ش� ���ҽ��� ã�Ƽ� �����Ѵ�.
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

