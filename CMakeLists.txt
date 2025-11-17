cmake_minimum_required(VERSION 3.13)
project(Physim)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------
# GLFW
# ----------------------------
set(GLFW_ROOT "${CMAKE_SOURCE_DIR}/libs/glfw")
add_subdirectory(${GLFW_ROOT})

# ----------------------------
# GLAD
# ----------------------------
add_library(glad "${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c")
target_include_directories(glad PUBLIC "${CMAKE_SOURCE_DIR}/libs/glad/include")

# ----------------------------
# Dear ImGui
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
# GLM / stb (header-only)
# ----------------------------
include_directories(
  "${CMAKE_SOURCE_DIR}/libs/glm"
  "${CMAKE_SOURCE_DIR}/libs/stb"
)

# ----------------------------
# Assimp (FetchContent)
# ----------------------------
include(FetchContent)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG master
)
FetchContent_MakeAvailable(assimp)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# ----------------------------
# Add engine + editor
# ----------------------------
add_subdirectory(engine)
add_subdirectory(editor)
