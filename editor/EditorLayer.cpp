#include "EditorLayer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

EditorLayer::EditorLayer(Window* window)
    : m_Window(window) {
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
}

void EditorLayer::OnRender()
{
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.39f, 0.58f, 0.93f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void EditorLayer::SetupImGuiFonts(const char* fontPath)
{
    ImGuiIO& io = ImGui::GetIO();

    float xscale, yscale;
    glfwGetWindowContentScale(m_Window->GetNativeWindow(), &xscale, &yscale);

    float fontSize = 16.0f * xscale;

    io.Fonts->Clear();
    io.FontDefault = io.Fonts->AddFontFromFileTTF(fontPath, fontSize);
    io.Fonts->Build();
}