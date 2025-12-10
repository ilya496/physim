#include "EditorLayer.h"

#include <EventBus.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

EditorLayer::EditorLayer(Window* window)
    : m_Window(window)
{
}

EditorLayer::~EditorLayer()
{
}

void EditorLayer::OnAttach()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_Window->GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    SetupImGuiFonts("../JetBrainsMono-Regular.ttf");
}

void EditorLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::OnUpdate(float dt)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    BeginDockspace();
}

void EditorLayer::OnRender()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.39f, 0.58f, 0.93f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawAssetsPanel();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::SetupImGuiFonts(const char* path)
{
    ImGuiIO& io = ImGui::GetIO();

    float xs, ys;
    glfwGetWindowContentScale(m_Window->GetNativeWindow(), &xs, &ys);

    float fontSize = 16.0f * xs;

    io.Fonts->Clear();
    io.FontDefault = io.Fonts->AddFontFromFileTTF(path, fontSize);
    io.Fonts->Build();
}

void EditorLayer::BeginDockspace()
{
    static bool dockspaceOpen = true;
    static bool optFullscreen = true;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    if (optFullscreen)
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Dockspace Window", &dockspaceOpen, windowFlags);
    ImGui::PopStyleVar();

    ImGuiID dockspaceID = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                WindowCloseEvent e;
                EventBus::Publish(e);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void EditorLayer::DrawAssetsPanel()
{
}