#include "FileDialog.h"
#include "../ImGui/Source/imgui_internal.h"
#include "../ImGui/Source/imgui_stdlib.h"
#include "Rendering/Model.h"

using namespace std;
using namespace PlayGround;
using namespace PlayGround::Math;

static const Vector2 default_size = Vector2(640.0f, 480.0f);

// ���� ��ũ��

#define OPERATION_NAME (m_Operation == FileDialog_Op_Open) ? "Open" : (m_Operation == FileDialog_Op_Load) ? "Load" : (m_Operation == FileDialog_Op_Save) ? "Save" : "View"
#define FILTER_NAME (m_Filter == FileDialog_Filter_All) ? "All (*.*)" : (m_Filter == FileDialog_Filter_Model) ? "Model(*.*)" : "World (*.world)"

FileDialog::FileDialog(Context* context, const bool standalone_window, const FileDialog_Type type, const FileDialog_Operation operation, const FileDialog_Filter filter)
{
    // ���� ���̾�α� �ʱ�ȭ
	m_Context = context;
	m_Type = type;
	m_Operation = operation;
	m_Filter = filter;
	m_Title = OPERATION_NAME;
	m_IsWindow = standalone_window;
	m_ItemSize = Vector2(100.0f, 100.0f);
	m_IsDirty = true;
	m_SelectionMode = false;
	m_Callback_on_item_clicked = nullptr;
	m_Callback_on_item_double_clicked = nullptr;
	m_Navigation.Navigate(m_Context->GetSubModule<ResourceCache>()->GetProjectDirectory());

	m_Position.x = PlayGround::Display::GetWidth() * 0.5f;
	m_Position.y = PlayGround::Display::GetHeight() * 0.5f;
}

void FileDialog::SetOperation(const FileDialog_Operation operation)
{
    // ���� ���̾�α� �۾� ����
	m_Operation = operation;
	m_Title = OPERATION_NAME;
}

bool FileDialog::Show(bool* is_visible, string* directory /*= nullptr*/, string* file_path /*= nullptr*/)
{
    // ���� �������� �ʴ´ٸ� �׳� ��ȯ
	if (!(*is_visible))
	{
		m_IsDirty = true;
		return false;
	}

	m_SelectionMode = false;
	m_IsHoveringItem = false;
	m_IsHoveringWindow = false;

    // �� �߰� �Ʒ� ������ ���ʴ�� �����ش�.
	ShowTop(is_visible);
	ShowMiddle();
	ShowBottom(is_visible);

    // ������ ����� ��� ���⼭ ���̳���.
	if (m_IsWindow)
	{
		ImGui::End();
	}

    // ���̾�α� ����
	if (m_IsDirty)
	{
		DialogUpdateFromDirectory(m_Navigation.m_CurrentPath);
		m_IsDirty = false;
	}

	if (m_SelectionMode)
	{
        // ��θ� ������ ���
		if (directory)
			(*directory) = m_Navigation.m_CurrentPath;
        // ���� ����� ���
		if (file_path)
			(*file_path) = m_Navigation.m_CurrentPath + "/" + string(m_InputBox);
	}

	return m_SelectionMode;
}

void FileDialog::ShowTop(bool* is_visible)
{
    // ������ ������ ���
	if (m_IsWindow)
	{
		if (m_Position.x != -1.0f && m_Position.y != -1.0f)
		{
            // ���� ����� �ǹ� ����Ʈ�� ���Ѵ�.
			ImVec2 pivot_center = ImVec2(0.5f, 0.5f);
			ImGui::SetNextWindowPos(m_Position, 0, pivot_center);
			m_Position = -1;
		}

        // ������ ����
		ImGui::SetNextWindowSize(default_size, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(350.0f, 250.0f), ImVec2(FLT_MAX, FLT_MAX));
		ImGui::Begin(m_Title.c_str(), is_visible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking);
		ImGui::SetWindowFocus();
	}

	{
        // ���� ��� ��ư
		if (ImGuiEX::Button("<"))
		{
			m_IsDirty = m_Navigation.Backward();
		}

		ImGui::SameLine();

        // ���� ��� ��ư
		if (ImGuiEX::Button(">"))
		{
			m_IsDirty = m_Navigation.Forward();
		}

        // ��� �����丮�� �ݺ��ϸ鼭 ��ư�� �����ش�.
		for (uint32_t i = 0; i < m_Navigation.m_vecPathHierarchy.size(); i++)
		{
			ImGui::SameLine();
			if (ImGuiEX::Button(m_Navigation.m_vecPathHierarchyLabels[i].c_str()))
				m_IsDirty = m_Navigation.Navigate(m_Navigation.m_vecPathHierarchy[i]);
		}
	}

	const float slider_width = 200.0f;
	ImGui::SameLine(ImGuiEX::GetWindowContentRegionWidth() - slider_width);
	ImGui::PushItemWidth(slider_width);
	const float prev_width = m_ItemSize.x;
	ImGui::SliderFloat("##FileDialogSlider", &m_ItemSize.x, m_MaxItemSize, m_MinItemSize);
	m_ItemSize.y += m_ItemSize.x - prev_width;
	ImGui::PopItemWidth();


	const float label_width = 37.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
	m_SearchFilter.Draw("Filter", ImGui::GetContentRegionAvail().x - label_width);
	ImGui::PopStyleVar();

	ImGui::Separator();
}

void FileDialog::ShowMiddle()
{
    const auto window = ImGui::GetCurrentWindowRead();
    const float content_width = ImGui::GetContentRegionAvail().x;
    const float content_height = ImGui::GetContentRegionAvail().y - m_OffsetBottom;
    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = ImGui::GetStyle();
    const float font_height = g.FontSize;
    const float label_height = font_height;
    const float text_offset = 3.0f;
    float pen_x_min = 0.0f;
    float pen_x = 0.0f;
    bool new_line = true;
    m_Displayed_item_count = 0;
    ImRect rect_button;
    ImRect rect_label;

    // �µθ� ����
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

    // �޹�� ��Ӱ� ����
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(
        static_cast<int>(m_ContentBGColor.x),
        static_cast<int>(m_ContentBGColor.y),
        static_cast<int>(m_ContentBGColor.z),
        static_cast<int>(m_ContentBGColor.w)));

    // ����
    if (ImGui::BeginChild("##ContentRegion", ImVec2(content_width, content_height), true))
    {
        m_IsHoveringWindow = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) ? true : m_IsHoveringWindow;

        // ���� ��ġ�� ���Ѵ�.
        {
            float offset = ImGui::GetStyle().ItemSpacing.x;
            pen_x_min = ImGui::GetCursorPosX() + offset;
            ImGui::SetCursorPosX(pen_x_min);
        }

        // ��� �������� ��ȸ�Ѵ�.
        for (int i = 0; i < m_vecItems.size(); i++)
        {
            // ������ ���÷��� ����
            auto& item = m_vecItems[i];

            // ����
            if (!m_SearchFilter.PassFilter(item.GetLabel().c_str()))
                continue;

            // ī��Ʈ �߰�
            m_Displayed_item_count++;

            // ���� �������� ���� ���η� �ʹ� ���� ������ �Ѿ�ٸ�
            // ���ο� ���ο��� �����Ѵ�.
            if (new_line)
            {
                ImGui::BeginGroup();
                new_line = false;
            }

            ImGui::BeginGroup();
            {
                // �簢�� ũ�� ���
                {
                    rect_button = ImRect
                    (
                        ImGui::GetCursorScreenPos().x,
                        ImGui::GetCursorScreenPos().y,
                        ImGui::GetCursorScreenPos().x + m_ItemSize.x,
                        ImGui::GetCursorScreenPos().y + m_ItemSize.y
                    );

                    rect_label = ImRect
                    (
                        rect_button.Min.x,
                        rect_button.Max.y - label_height - style.FramePadding.y,
                        rect_button.Max.x,
                        rect_button.Max.y
                    );
                }

                // �׸��� ȿ��
                if (m_DropShadow)
                {
                    static const float shadow_thickness = 2.0f;
                    ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_BorderShadow];
                    ImGui::GetWindowDrawList()->AddRectFilled(rect_button.Min, ImVec2(rect_label.Max.x + shadow_thickness, rect_label.Max.y + shadow_thickness), IM_COL32(color.x * 255, color.y * 255, color.z * 255, color.w * 255));
                }

                // �����
                {
                    ImGui::PushID(i);
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.25f));

                    if (ImGuiEX::Button("##dummy", m_ItemSize))
                    {
                        // ���� Ŭ�� ����Ȯ��
                        item.Clicked();
                        const bool is_single_click = item.GetTimeSinceLastClickMS() > 500;

                        // �ѹ� Ŭ������ ���
                        if (is_single_click)
                        {
                            // �Է� ���� ������Ʈ
                            m_InputBox = item.GetLabel();
                            // �ݹ� ȣ��
                            if (m_Callback_on_item_clicked) 
                                m_Callback_on_item_clicked(item.GetPath());
                        }
                        else // ���� Ŭ���ÿ�
                        {
                            m_IsDirty = m_Navigation.Navigate(item.GetPath());
                            m_SelectionMode = !item.IsDirectory();

                            // ��� �̵�
                            if (m_Type == FileDialog_Type_Browser)
                            {
                                if (!item.IsDirectory())
                                {
                                    FileSystem::OpenDirectoryWindow(item.GetPath());
                                }
                            }

                            // �ݹ�
                            if (m_Callback_on_item_double_clicked)
                            {
                                m_Callback_on_item_double_clicked(m_Navigation.m_CurrentPath);
                            }
                        }
                    }

                    // ������ ���
                    {
                        // ȣ���� ����
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
                        {
                            m_IsHoveringItem = true;
                            m_Hovered_item_path = item.GetPath();
                        }

                        ItemClick(&item);
                        ItemContextMenu(&item);
                        ItemDrag(&item);
                    }

                    // �̹���
                    {
                        // ����� ũ�� ���
                        ImVec2 image_size_max = ImVec2(rect_button.Max.x - rect_button.Min.x - style.FramePadding.x * 2.0f, rect_button.Max.y - rect_button.Min.y - style.FramePadding.y - label_height - 5.0f);
                        ImVec2 image_size = item.GetTexture() ? ImVec2(static_cast<float>(item.GetTexture()->GetWidth()), static_cast<float>(item.GetTexture()->GetHeight())) : image_size_max;
                        ImVec2 image_size_delta = ImVec2(0.0f, 0.0f);

                        // �ִ� ũ�⸦ ����Ѵ�.
                        {
                            // �ʺ� �ִ�ġ
                            if (image_size.x != image_size_max.x)
                            {
                                float scale = image_size_max.x / image_size.x;
                                image_size.y = image_size.y * scale;
                                image_size.x = image_size_max.x;
                            }

                            // ���� �ִ�ġ
                            if (image_size.y != image_size_max.y)
                            {
                                float scale = image_size_max.y / image_size.y;
                                image_size.x = image_size.x * scale;
                                image_size.y = image_size_max.y;
                            }

                            image_size_delta.x = image_size_max.x - image_size.x;
                            image_size_delta.y = image_size_max.y - image_size.y;
                        }

                        ImGui::SetCursorScreenPos(ImVec2(rect_button.Min.x + style.FramePadding.x + image_size_delta.x * 0.5f, rect_button.Min.y + style.FramePadding.y + image_size_delta.y * 0.5f));
                        ImGuiEX::Image(item.GetTexture(), image_size);
                    }

                    ImGui::PopStyleColor(2);
                    ImGui::PopID();
                }

                // ��
                {
                    const char* label_text = item.GetLabel().c_str();
                    const ImVec2 label_size = ImGui::CalcTextSize(label_text, nullptr, true);

                    // �� ���
                    ImGui::GetWindowDrawList()->AddRectFilled(rect_label.Min, rect_label.Max, IM_COL32(51, 51, 51, 190));

                    // �ؽ�Ʈ ���
                    ImGui::SetCursorScreenPos(ImVec2(rect_label.Min.x + text_offset, rect_label.Min.y + text_offset));
                    // ���� ���� ���̾�α� ũ�⸦ �Ѿ�� �ʾҴٸ�
                    if (label_size.x <= m_ItemSize.x && label_size.y <= m_ItemSize.y)
                    {
                        ImGui::TextUnformatted(label_text);
                    }
                    // �Ѿ�ٸ� Ŭ��
                    else
                    {
                        ImGui::RenderTextClipped(rect_label.Min, rect_label.Max, label_text, nullptr, &label_size, ImVec2(0, 0), &rect_label);
                    }
                }

                ImGui::EndGroup();
            }

            // ���� ��Ǵ� ���� �� ���� ����
            // x�� �̵�
            pen_x += m_ItemSize.x + ImGui::GetStyle().ItemSpacing.x;
            // ���� �̵��� x�� ��ġ�� �ִ�ġ�� �Ѿ�ٸ�
            if (pen_x >= content_width - m_ItemSize.x)
            {
                // �׷� ����
                ImGui::EndGroup();
                // �ٽ� �ּ�ġ�� ����
                pen_x = pen_x_min;
                // �ż� ����
                ImGui::SetCursorPosX(pen_x);
                // ������
                new_line = true;
            }
            // �ƴ϶�� ���ٿ� ��� ����Ѵ�.
            else
            {
                ImGui::SameLine();
            }
        }

        if (!new_line)
            ImGui::EndGroup();
    }

    // ImGui Begin Child�� ��Ʈ
    ImGui::EndChild();

    ImGui::PopStyleColor();

    ImGui::PopStyleVar();
}

void FileDialog::ShowBottom(bool* is_visible)
{
    // �� �Ʒ���

    // ������ Ÿ���� ���
    if (m_Type == FileDialog_Type_Browser)
    {
        m_OffsetBottom = 20.0f;
        ImGui::SetCursorPosY(ImGui::GetWindowSize().y - m_OffsetBottom);

        // ���õ� �������� ���� ���
        const char* text = (m_Displayed_item_count == 1) ? "%d item" : "%d items";
        ImGui::Text(text, m_Displayed_item_count);
    }
    // �ƴ� ��� ���͸� ����Ѵ�.
    else
    {
        m_OffsetBottom = 35.0f;
        ImGui::SetCursorPosY(ImGui::GetWindowSize().y - m_OffsetBottom);

        ImGui::PushItemWidth(ImGui::GetWindowSize().x - 235.0f);
        ImGui::InputText("##InputBox", &m_InputBox);
        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::Text(FILTER_NAME);

        ImGui::SameLine();

        if (ImGuiEX::Button(OPERATION_NAME))
        {
            m_SelectionMode = true;
        }

        ImGui::SameLine();
        if (ImGuiEX::Button("Cancel"))
        {
            m_SelectionMode = false;
            (*is_visible) = false;
        }
    }
}

void FileDialog::ItemDrag(FileDialogItem* item) const
{
    if (!item || m_Type != FileDialog_Type_Browser)
        return;

    // �巡�� ��� ����
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        // ���̷ε� ����
        const auto set_payload = [this](const ImGuiEX::DragPayloadType type, const string& path)
        {
            m_DragDrop_payload.type = type;
            m_DragDrop_payload.data = path.c_str();
            ImGuiEX::CreateDragPayload(m_DragDrop_payload);
        };

        if (FileSystem::IsSupportedModelFile(item->GetPath())) { set_payload(ImGuiEX::DragPayloadType::DragPayload_Model, item->GetPath()); }
        if (FileSystem::IsSupportedImageFile(item->GetPath())) { set_payload(ImGuiEX::DragPayloadType::DragPayload_Texture, item->GetPath()); }
        if (FileSystem::IsSupportedAudioFile(item->GetPath())) { set_payload(ImGuiEX::DragPayloadType::DragPayload_Audio, item->GetPath()); }
        if (FileSystem::IsEngineMaterialFile(item->GetPath())) { set_payload(ImGuiEX::DragPayloadType::DragPayload_Material, item->GetPath()); }

        ImGuiEX::Image(item->GetTexture(), 50.0f, false);

        ImGui::EndDragDropSource();
    }
}

void FileDialog::ItemClick(FileDialogItem* item) const
{
    // ���� ������ ���ų� ȣ���� ���� ���� ���¿����� Ŭ���� �Ұ���
    if (!item || !m_IsHoveringWindow)
        return;

    // ������ Ŭ�� ����
    if (ImGui::IsItemClicked(1))
    {
        m_ContextMenuID = item->GetID();
        ImGui::OpenPopup("##FileDialogContextMenu");
    }
}

void FileDialog::ItemContextMenu(FileDialogItem* item)
{
    // ������ Ŭ���� ������ �޴�

    if (m_ContextMenuID != item->GetID())
        return;

    if (!ImGui::BeginPopup("##FileDialogContextMenu"))
        return;


    // ����
    if (ImGui::MenuItem("Delete"))
    {
		FileSystem::Delete(item->GetPath());
		m_IsDirty = true;
    }

    ImGui::Separator();

    // ����
    if (ImGui::MenuItem("Open in file explorer"))
    {
        FileSystem::OpenDirectoryWindow(item->GetPath());
    }

    ImGui::EndPopup();
}

bool FileDialog::DialogUpdateFromDirectory(const string& file_path)
{
    // ��� üũ
    if (!FileSystem::IsDirectory(file_path))
    {
        LOG_ERROR("Provided path doesn't point to a directory.");
        return false;
    }

    // ������ �ʱ�ȭ
    m_vecItems.clear();
    m_vecItems.shrink_to_fit();

    // �ش� ��ο��� ��� ���� ��θ� �����´�.
    auto directories = FileSystem::GetDirectoriesInDirectory(file_path);
    // ������ �߰�
    for (const string& directory : directories)
    {
        m_vecItems.emplace_back(directory, IconProvider::Get().LoadFromFile(directory, EIconType::Directory_Folder, static_cast<uint32_t>(m_ItemSize.x)));
    }

    // ���Ͱ� ��ü ������ ���
    if (m_Filter == FileDialog_Filter_All)
    {
        vector<string> paths_anything = FileSystem::GetFilesInDirectory(file_path);

        for (const string& anything : paths_anything)
        {
            if (!FileSystem::IsEngineTextureFile(anything) && !FileSystem::IsEngineModelFile(anything))
            {
                m_vecItems.emplace_back(anything, IconProvider::Get().LoadFromFile(anything, EIconType::NotAssigned, static_cast<uint32_t>(m_ItemSize.x)));
            }
        }
    }
    // ������ ���
    else if (m_Filter == FileDialog_Filter_World)
    {
        vector<string> paths_world = FileSystem::GetSupportedSceneFilesInDirectory(file_path);
        for (const string& world : paths_world)
        {
            m_vecItems.emplace_back(world, IconProvider::Get().LoadFromFile(world, EIconType::Directory_File_World, static_cast<uint32_t>(m_ItemSize.x)));
        }
    }
    // ���� ���
    else if (m_Filter == FileDialog_Filter_Model)
    {
        vector<string> paths_models = FileSystem::GetSupportedModelFilesInDirectory(file_path);
        for (const string& model : paths_models)
        {
            m_vecItems.emplace_back(model, IconProvider::Get().LoadFromFile(model, EIconType::Directory_File_Model, static_cast<uint32_t>(m_ItemSize.x)));
        }
    }

    return true;
}