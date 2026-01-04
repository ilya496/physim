add_library(engine STATIC
    render/Shader.cpp
    render/Model.cpp
    render/Buffer.cpp
    render/VertexArray.cpp
    render/Camera.cpp
    render/Renderer.cpp
    core/Application.cpp
    core/Window.cpp
    core/LayerStack.cpp
    core/Timer.cpp
    core/EventBus.cpp
    core/RenderLayer.cpp
    core/Input.cpp
    physics/PhysicsWorld.cpp
    scene/Scene.cpp
    scene/Entity.cpp
    scene/SceneController.cpp
    scene/SceneSerializer.cpp
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
        EnTT::EnTT
        opengl32
)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
