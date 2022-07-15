#pragma once

#include "../IconProvider.h"
#include "Core/FileSystem.h"
#include "ImGuiHelper.h"

// ���� ���̾�α� Ÿ��
enum FileDialog_Type
{
    FileDialog_Type_Browser,
    FileDialog_Type_FileSelection
};

// ���ϴ��̾�α� ����
enum FileDialog_Operation
{
    FileDialog_Op_Open,
    FileDialog_Op_Load,
    FileDialog_Op_Save
};

// ���� ���̾�α� ����
enum FileDialog_Filter
{
    FileDialog_Filter_All,
    FileDialog_Filter_World,
    FileDialog_Filter_Model
};

// �׺���̼� Ŭ����
class FileDialogNavigation
{
public:
    bool Navigate(std::string directory, bool update_history = true)
    {
        // ��ΰ� ��ȿ���� �ʴٸ� ��ȯ
        if (!PlayGround::FileSystem::IsDirectory(directory))
            return false;

        // ��� ���� ������ ����
        if (directory.back() == '/')
            directory.pop_back();

        // ���� ���� ��ζ� �����ϴٸ� ��ȯ
        if (m_CurrentPath == directory)
            return false;

        // ��� ����
        m_CurrentPath = directory;

        // �����丮 ����
        if (update_history)
        {
            m_vecPathHistory.emplace_back(m_CurrentPath);
            m_PathHistoryIndex++;
        }

        m_vecPathHierarchy.clear();
        m_vecPathHierarchyLabels.clear();

        std::size_t pos = m_CurrentPath.find('/');

        // ���� �����ø� ã�� ���ߴٸ� �׳� �߰��Ѵ�.
        if (pos == std::string::npos)
        {
            m_vecPathHierarchy.emplace_back(m_CurrentPath);
        }
        // �����ø� ã�Ҵٸ�
        else
        {
            std::size_t prev_pos = 0;
            while (true)
            {
                // �����ñ����� �κ� ���ڿ��� �����.
                m_vecPathHierarchy.emplace_back(m_CurrentPath.substr(0, pos));

                // ��ġ ����
                prev_pos = pos;
                pos = m_CurrentPath.find('/', pos + 1);

                // ���� �����ø� ã�� ���ߴٸ� �ְ� ���� Ż��
                if (pos == std::string::npos)
                {
                    m_vecPathHierarchy.emplace_back(m_CurrentPath);
                    break;
                }
            }
        }

        // ��� ���ϰ��踦 ���� ��ȸ�Ѵ�.
        for (const auto& path : m_vecPathHierarchy)
        {
            // �����ø� ã�´�.
            pos = path.find('/');
            // ã�����ߴٸ� 
            if (pos == std::string::npos)
                m_vecPathHierarchyLabels.emplace_back(path + " >");
            // ã�Ҵٸ�
            else
                m_vecPathHierarchyLabels.emplace_back(path.substr(path.find_last_of('/') + 1) + " >");
        }

        return true;
    }

    // ���� ���
    bool Backward()
    {
        // ���� �����丮�� ����ְų� �� �̻� ���� �ö� �� ���ٸ� ��ȯ
        if (m_vecPathHistory.empty() || (m_PathHistoryIndex - 1) < 0)
            return false;

        // Ž�� ����
        Navigate(m_vecPathHistory[--m_PathHistoryIndex], false);

        return true;
    }

    // ���� ���
    bool Forward()
    {
        // ���� �����丮�� ����ְų� �ε����� �� Ŀ���ٸ� ��ȯ
        if (m_vecPathHistory.empty() || (m_PathHistoryIndex + 1) >= static_cast<int>(m_vecPathHistory.size()))
            return false;

        // Ž�� ����
        Navigate(m_vecPathHistory[++m_PathHistoryIndex], false);

        return true;
    }

    std::string m_CurrentPath;
    std::vector<std::string> m_vecPathHierarchy;
    std::vector<std::string> m_vecPathHierarchyLabels;
    std::vector<std::string> m_vecPathHistory;
    int m_PathHistoryIndex = -1;
};

// ���� ���̾�α� ������
class FileDialogItem
{
public:
    FileDialogItem(const std::string& path, const sThumbnail& thumbnail)
    {
        // ���̾�α� ������ �ʱ�ȭ
        m_Path = path;
        m_Thumbnail = thumbnail;
        m_ID = PlayGround::EngineObject::GenerateObjectID();
        m_IsDirectory = PlayGround::FileSystem::IsDirectory(path);
        m_Label = PlayGround::FileSystem::GetFileNameFromFilePath(path);
    }

    inline const std::string& GetPath() const { return m_Path; }
    inline const std::string& GetLabel() const { return m_Label; }
    inline uint64_t GetID() const { return m_ID; }
    PlayGround::RHI_Texture* GetTexture() const { return IconProvider::Get().GetTextureByThumbnail(m_Thumbnail); }
    inline bool IsDirectory() const { return m_IsDirectory; }
    inline float GetTimeSinceLastClickMS() const { return static_cast<float>(m_TimeSince_last_click.count()); }

    // ���� Ŭ�� üũ�� ���� �Լ�
    void Clicked()
    {
        // ���� Ŭ���� �ð��� �����´�.
        const auto now = std::chrono::high_resolution_clock::now();
        // ������ Ŭ�� �ð��� ���� Ŭ�� �ð��� ���̸� ���Ѵ�.
        m_TimeSince_last_click = now - m_LastClickTime;
        // ������ Ŭ���ð� ����
        m_LastClickTime = now;
    }

private:
    sThumbnail m_Thumbnail;
    uint64_t m_ID;
    std::string m_Path;
    std::string m_Label;
    bool m_IsDirectory;
    std::chrono::duration<double, std::milli> m_TimeSince_last_click;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_LastClickTime;
};

// ���� ���̾�α�
class FileDialog
{
public:
    FileDialog(PlayGround::Context* context, const bool standalone_window, const FileDialog_Type type, const FileDialog_Operation operation, const FileDialog_Filter filter);

    inline FileDialog_Type GetType() const { return m_Type; }
    inline FileDialog_Filter GetFilter() const { return m_Filter; }

    inline FileDialog_Operation GetOperation() const { return m_Operation; }
    void SetOperation(FileDialog_Operation operation);

    bool Show(bool* is_visible, std::string* directory = nullptr, std::string* file_path = nullptr);

    // ������ Ŭ���� �ݹ� �Լ�
    inline void SetCallbackOnItemClicked(const std::function<void(const std::string&)>& callback) { m_Callback_on_item_clicked = callback; }
    inline void SetCallbackOnItemDoubleClicked(const std::function<void(const std::string&)>& callbaack) { m_Callback_on_item_double_clicked = callbaack; }

private:
    // ���� ���̾�α״� �� 3���� ��Ʈ�� ���������ִ�.

    // ���� ���
    void ShowTop(bool* is_visible);
    // �� �߰�
    void ShowMiddle();
    // ���� �Ʒ�
    void ShowBottom(bool* is_visible);
    void ItemDrag(FileDialogItem* item) const;
    void ItemClick(FileDialogItem* item) const;
    void ItemContextMenu(FileDialogItem* item);

    bool DialogUpdateFromDirectory(const std::string& path);

    PlayGround::Math::Vector2 m_Position = PlayGround::Math::Vector2(-1.0f, -1.0f);
    const bool m_DropShadow = true;
    const float m_MinItemSize = 50.0f;
    const float m_MaxItemSize = 200.0f;
    const PlayGround::Math::Vector4 m_ContentBGColor = PlayGround::Math::Vector4(0.0f, 0.0f, 0.0f, 50.0f);

    bool m_IsWindow;
    bool m_SelectionMode;
    bool m_IsDirty;
    bool m_IsHoveringItem;
    bool m_IsHoveringWindow;
    std::string m_Title;
    FileDialogNavigation m_Navigation;
    std::string m_InputBox;
    std::string m_Hovered_item_path;
    uint32_t m_Displayed_item_count;

    mutable uint64_t m_ContextMenuID;
    mutable ImGuiEX::sDragDropPayload m_DragDrop_payload;
    float m_OffsetBottom = 0.0f;
    FileDialog_Type m_Type;
    FileDialog_Operation m_Operation;
    FileDialog_Filter m_Filter;
    std::vector<FileDialogItem> m_vecItems;
    PlayGround::Math::Vector2 m_ItemSize;
    ImGuiTextFilter m_SearchFilter;
    PlayGround::Context* m_Context;

    std::function<void(const std::string&)> m_Callback_on_item_clicked;
    std::function<void(const std::string&)> m_Callback_on_item_double_clicked;
};

