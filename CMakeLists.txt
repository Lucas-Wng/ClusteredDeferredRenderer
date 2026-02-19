cmake_minimum_required(VERSION 3.31)
project(ClusteredDeferredRenderer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_library(glad src/glad.c)
target_include_directories(glad PUBLIC include)

set(IMGUI_SOURCES
        extern/imgui/imgui.cpp
        extern/imgui/imgui_draw.cpp
        extern/imgui/imgui_tables.cpp
        extern/imgui/imgui_widgets.cpp
        extern/imgui/backends/imgui_impl_glfw.cpp
        extern/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC extern/imgui extern/imgui/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

add_executable(ClusteredDeferredRenderer src/main.cpp
        src/ModelLoader.cpp
        src/ModelLoader.h
        src/Application.cpp
        src/Application.h
        src/Scene.cpp
        src/Scene.h
        src/CameraController.cpp
        src/CameraController.h
        src/WindowCallbacks.cpp
        src/WindowCallbacks.h
        src/DeferredRenderer.cpp
        src/DeferredRenderer.h
)

target_include_directories(ClusteredDeferredRenderer PUBLIC include)
target_link_libraries(ClusteredDeferredRenderer glad imgui glfw OpenGL::GL)

enable_testing()

find_package(GTest CONFIG QUIET)
if (NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif ()

add_executable(CameraUnitTests
        tests/camera_tests.cpp
        tests/cluster_math_tests.cpp
        tests/scene_math_tests.cpp
)
target_include_directories(CameraUnitTests PRIVATE include src)
target_link_libraries(CameraUnitTests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(CameraUnitTests)
