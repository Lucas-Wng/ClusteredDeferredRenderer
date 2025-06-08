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

class GLFWwindow;

class Application {
public:
    void run();
    CameraController* cameraController = nullptr;

private:
    void initWindow();
    void initGL();
    void initCallbacks();
    void processInput();

    GLFWwindow* window = nullptr;
    Shader* shader = nullptr;
    Scene* scene = nullptr;
    DeferredRenderer* renderer = nullptr;
    Camera camera{glm::vec3(0.0f, 0.0f, 0.0f)};


    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
};

#endif //CLUSTEREDDEFERREDRENDERER_APPLICATION_H
