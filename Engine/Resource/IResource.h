#pragma once

#include <memory>
#include "../Core/Context.h"
#include "../Core/FileSystem.h"
#include "../Core/EngineObject.h"
#include "../Log/Logger.h"

namespace PlayGround
{
    // 리소스의 타입
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

    // 모든 리소스 인터페이스 클래스
	class IResource : public EngineObject
	{
    public:
        IResource(Context* context, EResourceType type);
        virtual ~IResource() = default;

        // 리소스의 파일 경로를 설정한다.
        void SetResourceFilePath(const std::string& path)
        {
            const bool is_native_file = FileSystem::IsEngineMaterialFile(path) || FileSystem::IsEngineModelFile(path);
            
            // 만약 엔진 자체 포맷일 경우 체크를 하지 않는다.
            if (!is_native_file)
            {
                if (!FileSystem::IsFile(path))
                {
                    LOG_ERROR("\"%s\" is not a valid file path", path.c_str());
                    return;
                }
            }

            const std::string file_path_relative = FileSystem::GetRelativePath(path);

            // 만약 외부 파일일 경우
            if (!FileSystem::IsEngineFile(path))
            {
                m_Resource_file_path_foreign = file_path_relative;
                m_Resource_file_path_native = FileSystem::NativizeFilePath(file_path_relative);
            }
            // 자체 포맷 파일일 경우
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
        
        // IResource 가상 메서드
        virtual bool SaveToFile(const std::string& filePath) { return true; }

        // IResource 가상 메서드
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

