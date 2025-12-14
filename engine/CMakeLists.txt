add_library(engine STATIC
    render/Shader.cpp
    render/Model.cpp
    render/Buffer.cpp
    render/VertexArray.cpp
    core/Application.cpp
    core/Window.cpp
    core/LayerStack.cpp
    core/Timer.cpp
    core/EventBus.cpp
    core/RenderLayer.cpp
    scene/Scene.cpp
    scene/Entity.cpp
    asset/AssetManager.cpp
    asset/AssetImporter.cpp
    project/Project.cpp
    project/ProjectSerializer.cpp
    utils/FileDialog.cpp
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
