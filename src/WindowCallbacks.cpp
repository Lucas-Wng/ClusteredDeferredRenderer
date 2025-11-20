//
// Created by Lucas Wang on 2025-06-08.
//

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "WindowCallbacks.h"
#include "Application.h"

void WindowCallbacks::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void WindowCallbacks::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app->isCursorCaptured()) return;
    app->cameraController->processMouse(xpos, ypos);
}

void WindowCallbacks::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->cameraController->processScroll(yoffset);
}
