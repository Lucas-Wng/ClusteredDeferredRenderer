cmake_minimum_required(VERSION 3.31)
project(ClusteredDeferredRenderer)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(include)
add_library(glad src/glad.c)

add_executable(ClusteredDeferredRenderer src/main.cpp
        src/ModelLoader.cpp
        src/ModelLoader.h)
target_link_libraries(ClusteredDeferredRenderer glad glfw OpenGL::GL)
