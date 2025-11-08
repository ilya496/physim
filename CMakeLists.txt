cmake_minimum_required(VERSION 3.10)
project(Physim)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------
# GLFW (build from source)
# ----------------------------
set(GLFW_ROOT "${CMAKE_SOURCE_DIR}/libs/glfw")
add_subdirectory(${GLFW_ROOT})
include_directories("${GLFW_ROOT}/include")

# ----------------------------
# GLAD (build from source)
# ----------------------------
add_library(glad "${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c")
target_include_directories(glad PUBLIC "${CMAKE_SOURCE_DIR}/libs/glad/include")

# ----------------------------
# Dear ImGui (build from source)
# ----------------------------
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/libs/imgui")

add_library(imgui
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

target_include_directories(imgui PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
    "${GLFW_ROOT}/include"
    "${CMAKE_SOURCE_DIR}/libs/glad/include"
)

# ----------------------------
# GLM & stb_image (header-only)
# ----------------------------
include_directories(
  "${CMAKE_SOURCE_DIR}/libs/glm"
  "${CMAKE_SOURCE_DIR}/libs/stb"
)

# ----------------------------
# Assimp (build from source using FetchContent)
# ----------------------------
include(FetchContent)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        master # You can pin to a version, e.g., "v5.3.1"
)
FetchContent_MakeAvailable(assimp)

# ----------------------------
# Application
# ----------------------------
add_executable(${PROJECT_NAME}
    main.cpp
    Shader.cpp
    Model.cpp
    Buffer.cpp
    VertexArray.cpp
)


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        imgui
        glad
        glfw
        assimp
        opengl32
)
