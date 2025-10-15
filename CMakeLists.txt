cmake_minimum_required(VERSION 3.10)
project(Physim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------
# GLFW (precompiled)
# ----------------------------
set(GLFW_ROOT "${CMAKE_SOURCE_DIR}/libs/glfw")
include_directories("${GLFW_ROOT}/include")
link_directories("${GLFW_ROOT}/lib-vc2022")

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
# Application
# ----------------------------
add_executable(${PROJECT_NAME} main.cpp)

# Link everything together
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        imgui
        glad
        glfw3
        opengl32
)
