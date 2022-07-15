#pragma once

#include <memory>
#include "../Core/Context.h"
#include "../Core/FileSystem.h"
#include "../Core/EngineObject.h"
#include "../Log/Logger.h"

namespace PlayGround
{
    // ���ҽ��� Ÿ��
    enum class EResourceType
    {
        Unknown,
        Texture,
        Texture2d,
        Texture2dArray,
        TextureCube,
        Audio,
        Material,
        Mesh,
        Model,
        Cubemap,
        Animation,
        Font,
        Shader
    };

    // ��� ���ҽ� �������̽� Ŭ����
	class IResource : public EngineObject
	{
    public:
        IResource(Context* context, EResourceType type);
        virtual ~IResource() = default;

        // ���ҽ��� ���� ��θ� �����Ѵ�.
        void SetResourceFilePath(const std::string& path)
        {
            const bool is_native_file = FileSystem::IsEngineMaterialFile(path) || FileSystem::IsEngineModelFile(path);
            
            // ���� ���� ��ü ������ ��� üũ�� ���� �ʴ´�.
            if (!is_native_file)
            {
                if (!FileSystem::IsFile(path))
                {
                    LOG_ERROR("\"%s\" is not a valid file path", path.c_str());
                    return;
                }
            }

            const std::string file_path_relative = FileSystem::GetRelativePath(path);

            // ���� �ܺ� ������ ���
            if (!FileSystem::IsEngineFile(path))
            {
                m_Resource_file_path_foreign = file_path_relative;
                m_Resource_file_path_native = FileSystem::NativizeFilePath(file_path_relative);
            }
            // ��ü ���� ������ ���
            else
            {
                m_Resource_file_path_foreign.clear();
                m_Resource_file_path_native = file_path_relative;
            }

            m_ResourceName = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path_relative);
            m_ResourceDirectory = FileSystem::GetDirectoryFromFilePath(file_path_relative);
        }

        inline EResourceType GetResourceType() const { return m_ResourceType; }

        inline const char* GetResourceTypeCStr() const { return typeid(*this).name(); }

        inline bool HasFilePathNative() const { return !m_Resource_file_path_native.empty(); }

        inline const std::string& GetResourceFilePath() const { return m_Resource_file_path_foreign; }

        inline const std::string& GetResourceFilePathNative() const { return m_Resource_file_path_native; }

        inline const std::string& GetResourceName() const { return m_ResourceName; }

        inline const std::string& GetResourceDirectory() const { return m_ResourceDirectory; }

        inline bool IsLoading() const { return m_IsLoading; }
        
        // IResource ���� �޼���
        virtual bool SaveToFile(const std::string& filePath) { return true; }

        // IResource ���� �޼���
        virtual bool LoadFromFile(const std::string& file_path) { return true; }

        template <typename T>
        static constexpr EResourceType TypeToEnum();

    protected:
        EResourceType m_ResourceType = EResourceType::Unknown;
        std::atomic<bool> m_IsLoading = false;

    private:
        std::string m_ResourceName;
        std::string m_ResourceDirectory;
        std::string m_Resource_file_path_native;
        std::string m_Resource_file_path_foreign;
	};
}

