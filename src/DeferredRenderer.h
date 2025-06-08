//
// Created by Lucas Wang on 2025-06-08.
//

#ifndef CLUSTEREDDEFERREDRENDERER_DEFERREDRENDERER_H
#define CLUSTEREDDEFERREDRENDERER_DEFERREDRENDERER_H

#include <glad/glad.h>
#include "shader.h"
#include "Scene.h"
#include "camera.h"

class DeferredRenderer {
public:
    DeferredRenderer(int width, int height);
    ~DeferredRenderer();

    void geometryPass(const Scene& scene, const Camera& camera);
    void lightingPass(const Camera& camera);

private:
    void initGBuffer();

    GLuint gBuffer;
    GLuint gPosition, gNormal, gAlbedoSpec;
    GLuint rboDepth;

    Shader geometryShader;
    Shader lightingShader;

    int screenWidth, screenHeight;
    GLuint quadVAO = 0, quadVBO = 0;

    void renderQuad();
};

#endif //CLUSTEREDDEFERREDRENDERER_DEFERREDRENDERER_H
