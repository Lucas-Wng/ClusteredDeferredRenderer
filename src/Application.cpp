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
    scene->loadModel(modelPathBuffer);
    lastLoadedModel = modelPathBuffer;
    renderer = new DeferredRenderer(SCR_WIDTH, SCR_HEIGHT, camera);
    cameraController = new CameraController(camera);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput();

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);

        if (width != renderer->getWidth() || height != renderer->getHeight()) {
            renderer->setScreenSize(width, height, camera);
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float currentTime = glfwGetTime();
        scene->updateLights(currentTime);

        renderer->geometryPass(*scene, camera);
        renderer->lightingPass(*scene, camera);

        ImGui::Begin("Debug Panel");
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::Separator();
        ImGui::InputText("Model Path", modelPathBuffer, IM_ARRAYSIZE(modelPathBuffer));
        if (ImGui::Button("Load glTF")) {
            scene->loadModel(modelPathBuffer);
            lastLoadedModel = modelPathBuffer;
        }
        ImGui::TextWrapped("Current model: %s", lastLoadedModel.c_str());
        ImGui::Separator();
        ImGui::Text("Add Light");
        ImGui::InputFloat3("Position", &newLightPos[0]);
        ImGui::DragFloat("Radius", &newLightRadius, 0.5f, 0.1f, 200.0f);
        ImGui::ColorEdit3("Color", &newLightColor[0]);
            ImGui::DragFloat("Intensity", &newLightIntensity, 0.1f, 0.1f, 20.0f);
        if (ImGui::Button("Add Light")) {
            scene->addLight(newLightPos, newLightRadius, newLightColor, newLightIntensity);
        }
        ImGui::Text("Total lights: %zu", scene->getLights().size());
        ImGui::Separator();
        ImGui::Text("Animate Lights");
        bool animatedLights = scene->getAnimate();
        if (ImGui::Button(animatedLights ? "ON" : "OFF")) {
            scene->setAnimate(!animatedLights);
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    shutdownImGui();
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
    glEnable(GL_FRAMEBUFFER_SRGB);
    initImGui();
}

void Application::initCallbacks() {
    glfwSetFramebufferSizeCallback(window, WindowCallbacks::framebufferSizeCallback);
    glfwSetCursorPosCallback(window, WindowCallbacks::mouseCallback);
    glfwSetScrollCallback(window, WindowCallbacks::scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Application::processInput() {
    cameraController->processKeyboard(window, deltaTime);
    int escapeState = glfwGetKey(window, GLFW_KEY_ESCAPE);
    if (escapeState == GLFW_PRESS && !escapePressedLastFrame) {
        cursorCaptured = !cursorCaptured;
        glfwSetInputMode(window, GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        cameraController->resetMouse();
    }
    escapePressedLastFrame = (escapeState == GLFW_PRESS);
}

void Application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void Application::shutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
