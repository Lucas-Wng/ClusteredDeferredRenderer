//
// Created by Lucas Wang on 2025-06-08.
//


#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Application.h"
#include "WindowCallbacks.h"
#include <iostream>

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

void Application::run() {
    initWindow();
    initGL();
    initCallbacks();

    scene = new Scene();
    scene->loadModel("assets/models/city/scene.gltf");
    renderer = new DeferredRenderer(SCR_WIDTH, SCR_HEIGHT);
    cameraController = new CameraController(camera);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer->geometryPass(*scene, camera);
        renderer->lightingPass(camera);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}

void Application::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ClusteredDeferredRenderer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
}

void Application::initGL() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }
    glEnable(GL_DEPTH_TEST);
}

void Application::initCallbacks() {
    glfwSetFramebufferSizeCallback(window, WindowCallbacks::framebufferSizeCallback);
    glfwSetCursorPosCallback(window, WindowCallbacks::mouseCallback);
    glfwSetScrollCallback(window, WindowCallbacks::scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Application::processInput() {
    cameraController->processKeyboard(window, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
