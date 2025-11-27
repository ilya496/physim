#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include "Camera.h"

#include "Model.h"

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    // FIXME: Introduce rendering on a separate thread
}

void SetupImGuiFonts(GLFWwindow* window, const char* font)
{
    ImGuiIO& io = ImGui::GetIO();

    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);

    float scale = xscale;
    float baseFontSize = 16.0f;
    float fontSize = baseFontSize * scale;

    io.Fonts->Clear();
    io.FontDefault = io.Fonts->AddFontFromFileTTF(font, fontSize);

    io.Fonts->Build();
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "ImGui + GLFW + GLAD", nullptr, nullptr);
    if (!window)
        return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double xpos, double ypos)

        { camera.HandleMouseMovement(w, xpos, ypos); });

    glfwSetScrollCallback(window, [](GLFWwindow* w, double xoffset, double yoffset)
        { camera.HandleScroll(xoffset, yoffset); });

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glViewport(0, 0, 1920, 1080);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    SetupImGuiFonts(window, "../JetBrainsMono-Regular.ttf");

    float planeVertices[] = {
        // positions             // normals          // texcoords
        -10.0f, -0.5f,  10.0f,     0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         10.0f, -0.5f,  10.0f,     0.0f, 1.0f, 0.0f,   10.0f, 0.0f,
         10.0f, -0.5f, -10.0f,     0.0f, 1.0f, 0.0f,   10.0f, 10.0f,

        -10.0f, -0.5f,  10.0f,     0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         10.0f, -0.5f, -10.0f,     0.0f, 1.0f, 0.0f,   10.0f, 10.0f,
        -10.0f, -0.5f, -10.0f,     0.0f, 1.0f, 0.0f,   0.0f, 10.0f
    };

    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    Shader shader("../shaders/default.vert", "../shaders/default.frag");
    Shader normalShader("../shaders/normals.vert", "../shaders/normals.frag", "../shaders/normals.geom");

    glm::vec3 lightPos = glm::vec3(1.2f, 1.0f, 2.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

    float deltaTime = 0.016f;
    float lastFrame = 0.0f;
    bool showNormals = false;

    auto geometry = Geometry::Generate(MeshPrimitive::CUBE);
    auto material = std::make_shared<PhongMaterial>();
    material->DiffuseMap = std::make_shared<Texture>("../brickwall.jpg");
    material->SpecularMap = std::make_shared<Texture>("../brickwall.jpg");
    material->NormalMap = std::make_shared<Texture>("../brickwall_normal.jpg");
    Mesh cube(geometry, material);

    auto planeGeometry = Geometry::Generate(MeshPrimitive::PLANE);
    Mesh plane(planeGeometry, material);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Lighting Controls");

        ImGui::Text("Light Settings");
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Light"))
        {
            ImGui::SliderFloat3("Position", glm::value_ptr(lightPos), -10.0f, 10.0f);
            ImGui::ColorEdit3("Color", glm::value_ptr(lightColor));
        }
        if (ImGui::CollapsingHeader("Debug"))
        {
            ImGui::Checkbox("Show normals", &showNormals);
        }

        ImGui::End();

        glEnable(GL_DEPTH_TEST);
        // glEnable(GL_CULL_FACE);
        glClearColor(0.39f, 0.58f, 0.93f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
        glm::mat4 normal = glm::transpose(glm::inverse(glm::mat3(model)));

        shader.Bind();

        shader.SetMat4f("u_Model", model);
        shader.SetMat4f("u_View", view);
        shader.SetMat4f("u_Projection", projection);
        shader.SetMat3f("u_Normal", normal);

        shader.SetVec3f("u_LightPos", lightPos);
        shader.SetVec3f("u_ViewPos", camera.Position);
        shader.SetVec3f("u_LightColor", lightColor);

        cube.Draw(shader);

        if (showNormals)
        {
            normalShader.Bind();
            normalShader.SetMat4f("u_Model", model);
            normalShader.SetMat4f("u_View", camera.GetViewMatrix());
            normalShader.SetMat4f("u_Projection", projection);
            shader.SetMat3f("u_Normal", normal);
            cube.Draw(normalShader);
        }

        shader.Bind();
        shader.SetMat4f("u_Model", glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f)));
        plane.Draw(shader);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}