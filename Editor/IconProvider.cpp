#include "Common.h"
#include "IconProvider.h"
#include "Rendering/Model.h"
#include "Widgets/ImGuiHelper.h"


using namespace std;
using namespace PlayGround;

static sThumbnail g_NoThumbnail;

IconProvider::IconProvider()
{
	m_Context = nullptr;
}

IconProvider::~IconProvider()
{
	m_vecThumbnails.clear();
}

void IconProvider::Initialize(Context* context)
{
    // 파일 로드

	m_Context = context;
	const string data_dir = m_Context->GetSubModule<ResourceCache>()->GetResourceDirectory() + "/";

    LoadFromFile(data_dir + "icons/options.png", EIconType::Component_Options);
    LoadFromFile(data_dir + "icons/console_info.png", EIconType::Console_Info);
    LoadFromFile(data_dir + "icons/console_warning.png", EIconType::Console_Warning);
    LoadFromFile(data_dir + "icons/console_error.png", EIconType::Console_Error);
    LoadFromFile(data_dir + "icons/button_play.png", EIconType::Button_Play);
    LoadFromFile(data_dir + "icons/button_pause.png", EIconType::Button_Pause);
    LoadFromFile(data_dir + "icons/button_step_forward.png", EIconType::Button_StepForward);
    LoadFromFile(data_dir + "icons/folder.png", EIconType::Directory_Folder);
    LoadFromFile(data_dir + "icons/world.png", EIconType::Directory_File_World);
    LoadFromFile(data_dir + "icons/model.png", EIconType::Directory_File_Model);
    LoadFromFile(data_dir + "icons/remove.png", EIconType::Component_Material_RemoveTexture);
}

RHI_Texture* IconProvider::GetTextureByType(EIconType type)
{
    return LoadFromFile("", type).texture.get();
}

RHI_Texture* IconProvider::GetTextureByFilePath(const string& file_path)
{
    return LoadFromFile(file_path).texture.get();
}

RHI_Texture* IconProvider::GetTextureByThumbnail(const sThumbnail& thumbnail)
{
    for (const sThumbnail& t : m_vecThumbnails)
    {
        if (t.texture->IsLoading())
            continue;

        if (t.texture->GetObjectID() == thumbnail.texture->GetObjectID())
            return t.texture.get();
    }

    return nullptr;
}

const sThumbnail& IconProvider::LoadFromFile(const std::string& file_path, EIconType type /*NotAssigned*/, const uint32_t size /*100*/)
{
    bool search_by_type = type != EIconType::NotAssigned;

    // 이미 파일이 존재하는 경우는 찾아서 반환한다.
    for (sThumbnail& t : m_vecThumbnails)
    {
        if (search_by_type)
        {
            if (t.type == type)
                return t;
        }
        else if (t.file_path == file_path)
            return t;
    }

    // 아닐경우는 지원 포맷일 경우에만 로딩한다.
    if (FileSystem::IsSupportedImageFile(file_path) || FileSystem::IsEngineTextureFile(file_path))
    {
        auto texture = make_shared<RHI_Texture2D>(m_Context, RHI_Texture_Flags::RHI_Texture_Srv);
        texture->SetWidth(size);
        texture->SetHeight(size);

        m_Context->GetSubModule<Threading>()->AddTask([texture, file_path]()
        {
            texture->LoadFromFile(file_path);
        });

        m_vecThumbnails.emplace_back(type, texture, file_path);
        return m_vecThumbnails.back();
    }

    return GetThumbnailByType(EIconType::Default);
}

const sThumbnail& IconProvider::GetThumbnailByType(EIconType type)
{
    for (sThumbnail& t : m_vecThumbnails)
    {
        if (t.type == type)
            return t;
    }

    return g_NoThumbnail;
}