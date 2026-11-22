cmake_minimum_required(VERSION 3.28)
project(3d_fluid_simulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# GLFW (windowing and input only; Direct3D does the drawing)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 GIT_SHALLOW TRUE)

# Dear ImGui (no CMake of its own, so built below)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.91.9 GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(glfw imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw d3d11 dxgi d3dcompiler)

# MSVC's default OpenMP is too old for unsigned loop counters; the LLVM runtime supports them
set(OpenMP_RUNTIME_MSVC llvm)
find_package(OpenMP REQUIRED)

add_executable(3d_fluid_simulation src/3d_fluid_simulation.cc src/Window.cc src/Graphics.cc src/Buffer.cc src/Shader.cc src/RenderShader.cc src/ConstantBuffer.cc src/Camera.cc src/Gui.cc src/Simulation.cc src/GpuSimulation.cc)
target_compile_definitions(3d_fluid_simulation PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN GLFW_INCLUDE_NONE)
target_link_libraries(3d_fluid_simulation PRIVATE glfw d3d11 dxgi d3dcompiler imgui OpenMP::OpenMP_CXX)



