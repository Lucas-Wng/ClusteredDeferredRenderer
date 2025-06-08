//
// Created by Lucas Wang on 2025-06-08.
//

#ifndef CLUSTEREDDEFERREDRENDERER_WINDOWCALLBACKS_H
#define CLUSTEREDDEFERREDRENDERER_WINDOWCALLBACKS_H

class GLFWwindow;

class WindowCallbacks {
public:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};


#endif //CLUSTEREDDEFERREDRENDERER_WINDOWCALLBACKS_H
