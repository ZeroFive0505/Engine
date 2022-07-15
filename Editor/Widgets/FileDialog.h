#pragma once

#include "../IconProvider.h"
#include "Core/FileSystem.h"
#include "ImGuiHelper.h"

// 파일 다이얼로그 타입
enum FileDialog_Type
{
    FileDialog_Type_Browser,
    FileDialog_Type_FileSelection
};

// 파일다이얼로그 동작
enum FileDialog_Operation
{
    FileDialog_Op_Open,
    FileDialog_Op_Load,
    FileDialog_Op_Save
};

// 파일 다이얼로그 필터
enum FileDialog_Filter
{
    FileDialog_Filter_All,
    FileDialog_Filter_World,
    FileDialog_Filter_Model
};

// 네비게이션 클래스
class FileDialogNavigation
{
public:
    bool Navigate(std::string directory, bool update_history = true)
    {
        // 경로가 유효하지 않다면 반환
        if (!PlayGround::FileSystem::IsDirectory(directory))
            return false;

        // 경로 끝에 슬래시 제거
        if (directory.back() == '/')
            directory.pop_back();

        // 만약 현재 경로랑 동일하다면 반환
        if (m_CurrentPath == directory)
            return false;

        // 경로 갱신
        m_CurrentPath = directory;

        // 히스토리 갱신
        if (update_history)
        {
            m_vecPathHistory.emplace_back(m_CurrentPath);
            m_PathHistoryIndex++;
        }

        m_vecPathHierarchy.clear();
        m_vecPathHierarchyLabels.clear();

        std::size_t pos = m_CurrentPath.find('/');

        // 만약 슬래시를 찾지 못했다면 그냥 추가한다.
        if (pos == std::string::npos)
        {
            m_vecPathHierarchy.emplace_back(m_CurrentPath);
        }
        // 슬래시를 찾았다면
        else
        {
            std::size_t prev_pos = 0;
            while (true)
            {
                // 슬래시까지의 부분 문자열을 만든다.
                m_vecPathHierarchy.emplace_back(m_CurrentPath.substr(0, pos));

                // 위치 갱신
                prev_pos = pos;
                pos = m_CurrentPath.find('/', pos + 1);

                // 만약 슬래시를 찾지 못했다면 넣고 루프 탈출
                if (pos == std::string::npos)
                {
                    m_vecPathHierarchy.emplace_back(m_CurrentPath);
                    break;
                }
            }
        }

        // 경로 상하관계를 전부 순회한다.
        for (const auto& path : m_vecPathHierarchy)
        {
            // 슬래시를 찾는다.
            pos = path.find('/');
            // 찾지못했다면 
            if (pos == std::string::npos)
                m_vecPathHierarchyLabels.emplace_back(path + " >");
            // 찾았다면
            else
                m_vecPathHierarchyLabels.emplace_back(path.substr(path.find_last_of('/') + 1) + " >");
        }

        return true;
    }

    // 상위 경로
    bool Backward()
    {
        // 만약 히스토리가 비어있거나 더 이상 위로 올라갈 수 없다면 반환
        if (m_vecPathHistory.empty() || (m_PathHistoryIndex - 1) < 0)
            return false;

        // 탐색 시작
        Navigate(m_vecPathHistory[--m_PathHistoryIndex], false);

        return true;
    }

    // 하위 경로
    bool Forward()
    {
        // 만약 히스토리가 비어있거나 인덱스가 더 커진다면 반환
        if (m_vecPathHistory.empty() || (m_PathHistoryIndex + 1) >= static_cast<int>(m_vecPathHistory.size()))
            return false;

        // 탐색 시작
        Navigate(m_vecPathHistory[++m_PathHistoryIndex], false);

        return true;
    }

    std::string m_CurrentPath;
    std::vector<std::string> m_vecPathHierarchy;
    std::vector<std::string> m_vecPathHierarchyLabels;
    std::vector<std::string> m_vecPathHistory;
    int m_PathHistoryIndex = -1;
};

// 파일 다이얼로그 아이템
class FileDialogItem
{
public:
    FileDialogItem(const std::string& path, const sThumbnail& thumbnail)
    {
        // 다이얼로그 아이템 초기화
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

    // 더블 클릭 체크를 위한 함수
    void Clicked()
    {
        // 현재 클릭한 시간을 가져온다.
        const auto now = std::chrono::high_resolution_clock::now();
        // 마지막 클릭 시간과 현재 클릭 시간의 차이를 비교한다.
        m_TimeSince_last_click = now - m_LastClickTime;
        // 마지막 클릭시간 갱신
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

// 파일 다이얼로그
class FileDialog
{
public:
    FileDialog(PlayGround::Context* context, const bool standalone_window, const FileDialog_Type type, const FileDialog_Operation operation, const FileDialog_Filter filter);

    inline FileDialog_Type GetType() const { return m_Type; }
    inline FileDialog_Filter GetFilter() const { return m_Filter; }

    inline FileDialog_Operation GetOperation() const { return m_Operation; }
    void SetOperation(FileDialog_Operation operation);

    bool Show(bool* is_visible, std::string* directory = nullptr, std::string* file_path = nullptr);

    // 아이템 클릭시 콜백 함수
    inline void SetCallbackOnItemClicked(const std::function<void(const std::string&)>& callback) { m_Callback_on_item_clicked = callback; }
    inline void SetCallbackOnItemDoubleClicked(const std::function<void(const std::string&)>& callbaack) { m_Callback_on_item_double_clicked = callbaack; }

private:
    // 파일 다이얼로그는 총 3개의 파트로 나뉘어져있다.

    // 제일 상단
    void ShowTop(bool* is_visible);
    // 그 중간
    void ShowMiddle();
    // 제일 아래
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

