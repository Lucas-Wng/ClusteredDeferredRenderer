//
// Created by Lucas Wang on 2025-06-08.
//

#ifndef CLUSTEREDDEFERREDRENDERER_CAMERACONTROLLER_H
#define CLUSTEREDDEFERREDRENDERER_CAMERACONTROLLER_H


#include "camera.h"

class GLFWwindow;

class CameraController {
public:
    CameraController(Camera& cam);
    void processKeyboard(GLFWwindow* window, float deltaTime);
    void processMouse(double xpos, double ypos);
    void processScroll(double yoffset);
    void resetMouse();
private:
    Camera& camera;
    float lastX = 400.0f;
    float lastY = 300.0f;
    bool firstMouse = true;
};

#endif //CLUSTEREDDEFERREDRENDERER_CAMERACONTROLLER_H
