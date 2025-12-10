add_library(engine STATIC
    Shader.cpp
    Model.cpp
    Buffer.cpp
    VertexArray.cpp
    Application.cpp
    Window.cpp
    LayerStack.cpp
    Timer.cpp
    scene/Scene.cpp
    scene/Entity.cpp
    EventBus.cpp
    asset/AssetManager.cpp
    asset/AssetImporter.cpp
    project/Project.cpp
    project/ProjectSerializer.cpp
)

target_link_libraries(engine
    PUBLIC
        glad
        imgui
        glfw
        assimp
        nlohmann_json::nlohmann_json
        opengl32
)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
