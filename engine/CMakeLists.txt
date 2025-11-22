add_library(engine STATIC
    Shader.cpp
    Model.cpp
    Buffer.cpp
    VertexArray.cpp
    Application.cpp
    Window.cpp
    UUID.cpp
    LayerStack.cpp
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
