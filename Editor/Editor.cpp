#include "Common.h"
#include "Editor.h"
#include "Core/Engine.h"
#include "Core/Settings.h"
#include "Core/Window.h"
#include "Core/SubModule.h"
#include "Core/EventSystem.h"
#include "../ThirdParty/SDL2-2.0.22/SDL.h"
#include "Rendering/Model.h"
#include "Profiling/Profiler.h"
#include "Widgets/ImGuiHelper.h"
#include "ImGui/Backend/imGui_RHI.h"
#include "ImGui/Backend/imgui_impl_sdl.h"
#include "Widgets/Console.h"
#include "Widgets/MenuBar.h"
#include "Widgets/ProgressDialog.h"
#include "Widgets/Properties.h"
#include "Widgets/Viewport.h"
#include "Widgets/WorldViewer.h"
#include "Widgets/ResourceViewer.h"
#include "Widgets/Profiler.h"
#include "Widgets/RenderOptions.h"
#include "Widgets/TextureViewer.h"

using namespace std;

const float g_roundness = 2.0f;
// ��Ʈ
const float g_font_size = 24.0f;
const float g_font_scale = 0.7f;


// ������ ���� ����
namespace _Editor
{
	MenuBar* widget_menu_bar = nullptr;
	Widget* widget_world = nullptr;
	PlayGround::Renderer* renderer = nullptr;
    PlayGround::RHI_SwapChain* swap_chain = nullptr;
	PlayGround::Profiler* profilier = nullptr;
	PlayGround::Window* window = nullptr;
	shared_ptr<PlayGround::RHI_Device> rhi_device;
}

// IMGui �ʱ�ȭ
static void ImGui_Init(PlayGround::Context* context)
{
	IMGUI_CHECKVERSION();
	context->GetSubModule<PlayGround::Settings>()->RegisterThirdParty("Dear ImGui", IMGUI_VERSION, "https://github.com/ocornut/imgui");


	ImGui::CreateContext();

    // IO �÷���
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigWindowsResizeFromEdges = true;
	io.ConfigViewportsNoTaskBarIcon = true;
	io.IniFilename = "editor.ini";
    
    // ������ ��Ʈ ����
	const string dir_fonts = context->GetSubModule<PlayGround::ResourceCache>()->GetResourceDirectory(PlayGround::EResourceDirectory::Fonts) + "/";
	io.Fonts->AddFontFromFileTTF((dir_fonts + "Roboto-Medium.ttf").c_str(), g_font_size);
	io.FontGlobalScale = g_font_scale;

    // ImGui SDL ����
	ImGui_ImplSDL2_Init(context);
    // RHI �ʱ�ȭ
	ImGui::RHI::Initialize(context);
}

// SDL �̺�Ʈ 
static void ImGui_ProcessEvent(const PlayGround::Variant& event_variant)
{
    // �̺�Ʈ ó��
	SDL_Event* event_sdl = event_variant.Get<SDL_Event*>();
	ImGui_ImplSDL2_ProcessEvent(event_sdl);
}

// ImGui ����
static void ImGui_Shutdown()
{
	if (ImGui::GetCurrentContext())
	{
		ImGui::RHI::Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}
}


static void ImGui_ApplyColors()
{
    ImGui::StyleColorsDark();       
}

static void ImGui_ApplyStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.ScrollbarSize = 20.0f;
    style.FramePadding = ImVec2(5, 5);
    style.ItemSpacing = ImVec2(6, 5);
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.WindowRounding = g_roundness;
    style.FrameRounding = g_roundness;
    style.PopupRounding = g_roundness;
    style.GrabRounding = g_roundness;
    style.ScrollbarRounding = g_roundness;
    style.Alpha = 1.0f;
}

Editor::Editor()
{
    // ���� ����
    m_Engine = make_unique<PlayGround::Engine>();

    // ���ؽ�Ʈ�� �ֿ� ��Ҹ� �����´�.
    m_Context = m_Engine->GetContext();
    _Editor::profilier = m_Context->GetSubModule<PlayGround::Profiler>();
    _Editor::renderer = m_Context->GetSubModule<PlayGround::Renderer>();
    _Editor::window = m_Context->GetSubModule<PlayGround::Window>();
    _Editor::rhi_device = _Editor::renderer->GetRhiDevice();
    _Editor::swap_chain = _Editor::renderer->GetSwapChain();

    // �ʱ�ȭ
    Initialize();

    // SDL �̺�Ʈ ����
    SUBSCRIBE_TO_EVENT(EventType::EventSDL, EVENT_HANDLER_VARIANT_STATIC(ImGui_ProcessEvent));
}

Editor::~Editor()
{
    ImGui_Shutdown();
}

void Editor::Update()
{
    // �����찡 ������ ������ ����ؼ� ������Ʈ�Ѵ�.
    while (!_Editor::window->WantsToClose())
    {
        // ���� ������Ʈ
        m_Engine->Update();

        if (_Editor::window->IsFullScreen())
        {
            _Editor::renderer->Pass_CopyToBackbuffer();
        }
        else
        {
            // Imgui ����
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            // ������ ����
            BeginWindow();

            // ������ ������Ʈ
            for (shared_ptr<Widget>& widget : m_vecWidgets)
                widget->Update();

            // ������ ����
            if (m_Editor_begun)
            {
                ImGui::End();
            }

            // ImGui ��, ����
            ImGui::Render();
            ImGui::RHI::Render(ImGui::GetDrawData());
        }

        // ���� ������ ����
        _Editor::renderer->Present();

        // �ڽ� ������ ������Ʈ
        if (!_Editor::window->IsFullScreen() && ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}

void Editor::Initialize()
{
    ImGui_Init(m_Context);
    ImGui_ApplyColors();
    ImGui_ApplyStyle();

    // ���ؽ�Ʈ �ʱ�ȭ
    IconProvider::Get().Initialize(m_Context);
    EditorHelper::Get().Initialize(m_Context);

    // ���� �߰�
    m_vecWidgets.emplace_back(make_shared<Console>(this));
    m_vecWidgets.emplace_back(make_shared<Profiler>(this));
    m_vecWidgets.emplace_back(make_shared<ResourceViewer>(this));
    m_vecWidgets.emplace_back(make_shared<RenderOptions>(this));
    m_vecWidgets.emplace_back(make_shared<TextureViewer>(this));
    m_vecWidgets.emplace_back(make_shared<MenuBar>(this)); 
    _Editor::widget_menu_bar = static_cast<MenuBar*>(m_vecWidgets.back().get());
    m_vecWidgets.emplace_back(make_shared<Viewport>(this));
    m_vecWidgets.emplace_back(make_shared<Properties>(this));
    m_vecWidgets.emplace_back(make_shared<WorldViewer>(this)); 
    _Editor::widget_world = m_vecWidgets.back().get();
    m_vecWidgets.emplace_back(make_shared<ProgressDialog>(this));
}

void Editor::BeginWindow()
{
    const int window_flags = ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    // ������ ��ġ�� ũ�⸦ ���Ѵ�.
    float offset_y = _Editor::widget_menu_bar ? (_Editor::widget_menu_bar->GetHeight() + _Editor::widget_menu_bar->GetPadding()) : 0;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + offset_y));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - offset_y));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // ������ ����
    string name = "##Main_window";
    bool open = true;
    m_Editor_begun = ImGui::Begin(name.c_str(), &open, window_flags);
    ImGui::PopStyleVar(3);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable && m_Editor_begun)
    {
        const auto window_id = ImGui::GetID(name.c_str());

        if (!ImGui::DockBuilderGetNode(window_id))
        {
            // ���� ��ŷ ������ �ʱ�ȭ�Ѵ�.
            ImGui::DockBuilderRemoveNode(window_id);
            ImGui::DockBuilderAddNode(window_id, ImGuiDockNodeFlags_None);
            ImGui::DockBuilderSetNodeSize(window_id, ImGui::GetMainViewport()->Size);

            ImGuiID dock_main_id = window_id;
            ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
            ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
            ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

            // ��ŷ
            ImGui::DockBuilderDockWindow("World", dock_left_id);
            ImGui::DockBuilderDockWindow("Properties", dock_right_id);
            ImGui::DockBuilderDockWindow("Console", dock_down_id);
            ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

            ImGui::DockBuilderFinish(dock_main_id);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::DockSpace(window_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::PopStyleVar();
    }
}