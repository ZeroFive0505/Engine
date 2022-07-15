#include "Common.h"
#include "ResourceCache.h"
#include "ProgressTracker.h"
#include "Importer/ImageImporter.h"
#include "Importer/ModelImporter.h"
#include "Importer/FontImporter.h"
#include "../World/World.h"
#include "../World/Entity.h"
#include "../IO/FileStream.h"
#include "../RHI/RHI_Texture2D.h"
#include "../RHI/RHI_Texture2DArray.h"
#include "../RHI/RHI_TextureCube.h"
#include "../Audio/AudioClip.h"
#include "../Rendering/Model.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	ResourceCache::ResourceCache(Context* context) : SubModule(context)
	{
        // ���ҽ� �⺻ ���
		const string data_dir = "Data\\";

        // ���ҽ� ��� �߰�
        AddResourceDirectory(EResourceDirectory::CubeMaps, data_dir + "environment");
        AddResourceDirectory(EResourceDirectory::Fonts, data_dir + "fonts");
        AddResourceDirectory(EResourceDirectory::Icons, data_dir + "icons");
        AddResourceDirectory(EResourceDirectory::ShaderCompiler, data_dir + "shader_compiler");
        AddResourceDirectory(EResourceDirectory::Shaders, data_dir + "shaders");
        AddResourceDirectory(EResourceDirectory::Textures, data_dir + "textures");

        // ������Ʈ ��� ����
        SetProjectDirectory("Project/");

        // ����, �ε�, Ŭ���� �̺�Ʈ�� �ش� �ڵ鷯�� ȣ���Ѵ�.
        SUBSCRIBE_TO_EVENT(EventType::WorldSaveStart, EVENT_HANDLER(SaveResourcesToFiles));
        SUBSCRIBE_TO_EVENT(EventType::WorldLoadStart, EVENT_HANDLER(LoadResourcesFromFiles));
        SUBSCRIBE_TO_EVENT(EventType::WorldClear, EVENT_HANDLER(Clear));
	}

    ResourceCache::~ResourceCache()
    {
        // ���� ����
        UNSUBSCRIBE_FROM_EVENT(EventType::WorldSaveStart, EVENT_HANDLER(SaveResourcesToFiles));
        UNSUBSCRIBE_FROM_EVENT(EventType::WorldLoadStart, EVENT_HANDLER(LoadResourcesFromFiles));
        UNSUBSCRIBE_FROM_EVENT(EventType::WorldClear, EVENT_HANDLER(Clear));
    }

    void ResourceCache::OnInit()
    {
        m_ImageImporter = make_shared<ImageImporter>(m_Context);
        m_ModelImporter = make_shared<ModelImporter>(m_Context);
        m_FontImporter = make_shared<FontImporter>(m_Context);
    }

    bool ResourceCache::IsCached(const string& resource_name, const EResourceType resource_type)
    {
        ASSERT(!resource_name.empty());

        // ĳ�̵Ǿ��ִ� ���ҽ����� Ȯ���Ѵ�.
        for (shared_ptr<IResource>& resource : m_vecResources)
        {
            if (resource->GetResourceType() != resource_type)
                continue;

            if (resource_name == resource->GetResourceName())
                return true;
        }

        return false;
    }

    bool ResourceCache::IsCached(const uint64_t resource_id)
    {
        // ���ҽ� ���̵� �������� ĳ�̵Ǿ� �ִ��� Ȯ���Ѵ�.
        for (shared_ptr<IResource>& resource : m_vecResources)
        {
            if (resource_id == resource->GetObjectID())
                return true;
        }

        return false;
    }

    shared_ptr<IResource>& ResourceCache::GetByName(const string& name, const EResourceType type)
    {
        // �̸��������� ĳ�� �Ǿ� �ִ��� Ȯ���Ѵ�.
        for (shared_ptr<IResource>& resource : m_vecResources)
        {
            if (name == resource->GetResourceName())
                return resource;
        }

        static shared_ptr<IResource> empty;
        return empty;
    }

    vector<shared_ptr<IResource>> ResourceCache::GetByType(const EResourceType type)
    {
        vector<shared_ptr<IResource>> resources;

        // ���ҽ� Ÿ�� �������� ĳ�̵Ǿ� �ִ��� Ȯ��
        for (shared_ptr<IResource>& resource : m_vecResources)
        {
            if (resource->GetResourceType() == type || type == EResourceType::Unknown)
            {
                resources.emplace_back(resource);
            }
        }

        return resources;
    }

    uint64_t ResourceCache::GetMemoryUsageCPU(EResourceType type)
    {
        // CPU������ ũ�⸦ ���Ѵ�.
        uint64_t size = 0;

        for (shared_ptr<IResource>& resource : m_vecResources)
        {
            if (resource->GetResourceType() == type || type == EResourceType::Unknown)
            {
                EngineObject* object = dynamic_cast<EngineObject*>(resource.get());
            
                // ũ�� ����
                if (object)
                {
                    size += object->GetObjectSizeCPU();
                }
            }
        }

        return size;
    }

    uint64_t ResourceCache::GetMemoryUsageGPU(EResourceType type)
    {
        // GPU������ ũ�⸦ ���Ѵ�.
        uint64_t size = 0;

        for (shared_ptr<IResource>& resource : m_vecResources)
        {
            if (resource->GetResourceType() == type || type == EResourceType::Unknown)
            {
                EngineObject* object = dynamic_cast<EngineObject*>(resource.get());

                // ������ ����
                if (object)
                {
                    size += object->GetObjectSizeGPU();
                }
            }
        }

        return size;
    }

    void ResourceCache::SaveResourcesToFiles()
    {
        // ���α׷��� Ʈ��Ŀ ����
        ProgressTracker::Get().Reset(EProgressType::ResoruceCache);
        ProgressTracker::Get().SetIsLoading(EProgressType::ResoruceCache, true);
        ProgressTracker::Get().SetStatus(EProgressType::ResoruceCache, "Loading resources...");

        // ���ҽ��� �����Ѵ�.
        string file_path = GetProjectDirectoryAbsolute() + m_Context->GetSubModule<World>()->GetName() + "_resources.dat";
        auto file = make_unique<FileStream>(file_path, FileStream_Write);


        if (!file->IsOpen())
        {
            LOG_ERROR("Failed to open file.");
            return;
        }

        // ���ҽ��� ���� �����´�.
        const uint32_t resource_count = GetResourceCount();
        ProgressTracker::Get().SetJobCount(EProgressType::ResoruceCache, resource_count);

        // ���ҽ� ����
        file->Write(resource_count);

        for (shared_ptr<IResource>& resource : m_vecResources)
        {
            // ��ü ������ �ƴҰ��� ����
            if (!resource->HasFilePathNative())
                continue;

            // ���� ���

            file->Write(resource->GetResourceFilePathNative());

            file->Write(static_cast<uint32_t>(resource->GetResourceType()));

            resource->SaveToFile(resource->GetResourceFilePathNative());

            ProgressTracker::Get().IncrementJobsDone(EProgressType::ResoruceCache);
        }

        // Ʈ��Ŀ ����
        ProgressTracker::Get().SetIsLoading(EProgressType::ResoruceCache, false);
    }

    void ResourceCache::LoadResourcesFromFiles()
    {
        // ���ҽ� ������ �ҷ��´�.
        string file_path = GetProjectDirectoryAbsolute() + m_Context->GetSubModule<World>()->GetName() + "_resources.dat";
        unique_ptr<FileStream> file = make_unique<FileStream>(file_path, FileStream_Read);

        if (!file->IsOpen())
            return;

        // ���ҽ��� ������ ���� �����´�.
        const uint32_t resource_count = file->ReadAs<uint32_t>();

        // ���ҽ��� ����ŭ �ݺ��Ѵ�.
        for (uint32_t i = 0; i < resource_count; i++)
        {
            string file_path = file->ReadAs<string>();

            const EResourceType type = static_cast<EResourceType>(file->ReadAs<uint32_t>());

            switch (type)
            {
            case EResourceType::Model:
                Load<Model>(file_path);
                break;
            case EResourceType::Material:
                Load<Material>(file_path);
                break;
            case EResourceType::Texture:
                Load<RHI_Texture>(file_path);
                break;
            case EResourceType::Texture2d:
                Load<RHI_Texture2D>(file_path);
                break;
            case EResourceType::Texture2dArray:
                Load<RHI_Texture2DArray>(file_path);
                break;
            case EResourceType::TextureCube:
                Load<RHI_TextureCube>(file_path);
                break;
            case EResourceType::Audio:
                Load<AudioClip>(file_path);
                break;
            }
        }
    }

    void ResourceCache::Clear()
    {
        // ������ Ŭ����
        uint32_t resource_count = static_cast<uint32_t>(m_vecResources.size());

        m_vecResources.clear();

        LOG_INFO("%d resources have been cleared", resource_count);
    }

    // ���ҽ��� ������ �����´�.
    uint32_t ResourceCache::GetResourceCount(const EResourceType type)
    {
        return static_cast<uint32_t>(GetByType(type).size());
    }

    // ���ҽ� ���丮 �߰�
    void ResourceCache::AddResourceDirectory(const EResourceDirectory type, const string& directory)
    {
        m_mapStandard_resource_directories[type] = directory;
    }

    // ���ҽ� ���丮�� �����´�.
    string ResourceCache::GetResourceDirectory(const EResourceDirectory type)
    {
        for (auto& directory : m_mapStandard_resource_directories)
        {
            if (directory.first == type)
                return directory.second;
        }

        return "";
    }

    // ������Ʈ ���丮 ����
    void ResourceCache::SetProjectDirectory(const string& directory)
    {
        if (!FileSystem::Exists(directory))
        {
            FileSystem::CreateDirectory_(directory);
        }

        m_ProjectDirectory = directory;
    }

    // ���� ��θ� �����´�.
    string ResourceCache::GetProjectDirectoryAbsolute() const
    {
        return FileSystem::GetWorkingDirectory() + "/" + m_ProjectDirectory;
    }
}