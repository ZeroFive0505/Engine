#pragma once

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include "RHI/RHI_Definition.h"

// ������ ������ Ÿ��
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

// ����� ����ü
struct sThumbnail
{
    sThumbnail() = default;
    sThumbnail(EIconType _type, std::shared_ptr<PlayGround::RHI_Texture> _texture, const std::string& _file_path)
    {
        type = _type;
        texture = std::move(_texture);
        file_path = _file_path;
    }
    
    // ������ Ÿ��
    EIconType type = EIconType::NotAssigned;
    // �ؽ���
    std::shared_ptr<PlayGround::RHI_Texture> texture;
    // ���� ���
    std::string file_path;
};

// ������ ���� Ŭ����
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

    // Ÿ������ ������ ��ȯ
    PlayGround::RHI_Texture* GetTextureByType(EIconType type);
    // ���� ��η� ������ ��ȯ
    PlayGround::RHI_Texture* GetTextureByFilePath(const std::string& file_path);
    // ����Ϸ� ������ ��ȯ
    PlayGround::RHI_Texture* GetTextureByThumbnail(const sThumbnail& thumbnail);
    // ���� �ε�
    const sThumbnail& LoadFromFile(const std::string& file_path, EIconType type = EIconType::NotAssigned, const uint32_t size = 100);

private:
    const sThumbnail& GetThumbnailByType(EIconType type);
    std::vector<sThumbnail> m_vecThumbnails;
    PlayGround::Context* m_Context;
};

