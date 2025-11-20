//
// Created by Lucas Wang on 2025-06-08.
//

#ifndef CLUSTEREDDEFERREDRENDERER_APPLICATION_H
#define CLUSTEREDDEFERREDRENDERER_APPLICATION_H


//#include <GLFW/glfw3.h>
#include "shader.h"
#include "Scene.h"
#include "CameraController.h"
#include "camera.h"
#include "DeferredRenderer.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <string>
#include <glm/glm.hpp>

class GLFWwindow;

class Application {
public:
    void run();
    CameraController* cameraController = nullptr;
    bool isCursorCaptured() const { return cursorCaptured; }

private:
    void initWindow();
    void initGL();
    void initCallbacks();
    void processInput();
    void initImGui();
    void shutdownImGui();

    GLFWwindow* window = nullptr;
    Shader* shader = nullptr;
    Scene* scene = nullptr;
    DeferredRenderer* renderer = nullptr;
    Camera camera{glm::vec3(0.0f, 0.0f, 2.0f)};


    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    bool cursorCaptured = true;
    bool escapePressedLastFrame = false;
    char modelPathBuffer[256] = "assets/models/backpack/scene.gltf";
    std::string lastLoadedModel = modelPathBuffer;
    glm::vec3 newLightPos = glm::vec3(0.0f, 2.0f, 0.0f);
    float newLightRadius = 10.0f;
    glm::vec3 newLightColor = glm::vec3(1.0f, 0.9f, 0.7f);
    float newLightIntensity = 3.0f;
};

#endif //CLUSTEREDDEFERREDRENDERER_APPLICATION_H
