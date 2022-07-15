#pragma once

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include "RHI/RHI_Definition.h"

// 에디터 아이콘 타입
enum class EIconType
{
    NotAssigned,
    Component_Options,
    Console_Info,
    Console_Warning,
    Console_Error,
    Component_Material_RemoveTexture,
    Button_Play,
    Button_Pause,
    Button_StepForward,
    Directory_Folder,
    Directory_File_World,
    Directory_File_Model,
    Default,
};

namespace PlayGround
{
    class Context;
}

// 썸네일 구조체
struct sThumbnail
{
    sThumbnail() = default;
    sThumbnail(EIconType _type, std::shared_ptr<PlayGround::RHI_Texture> _texture, const std::string& _file_path)
    {
        type = _type;
        texture = std::move(_texture);
        file_path = _file_path;
    }
    
    // 아이콘 타입
    EIconType type = EIconType::NotAssigned;
    // 텍스쳐
    std::shared_ptr<PlayGround::RHI_Texture> texture;
    // 파일 경로
    std::string file_path;
};

// 아이콘 관리 클래스
class IconProvider
{
public:
    static IconProvider& Get()
    {
        static IconProvider instance;
        return instance;
    }

    IconProvider();
    ~IconProvider();

    void Initialize(PlayGround::Context* context);

    // 타입으로 아이콘 반환
    PlayGround::RHI_Texture* GetTextureByType(EIconType type);
    // 파일 경로로 아이콘 반환
    PlayGround::RHI_Texture* GetTextureByFilePath(const std::string& file_path);
    // 썸네일로 아이콘 반환
    PlayGround::RHI_Texture* GetTextureByThumbnail(const sThumbnail& thumbnail);
    // 파일 로드
    const sThumbnail& LoadFromFile(const std::string& file_path, EIconType type = EIconType::NotAssigned, const uint32_t size = 100);

private:
    const sThumbnail& GetThumbnailByType(EIconType type);
    std::vector<sThumbnail> m_vecThumbnails;
    PlayGround::Context* m_Context;
};

