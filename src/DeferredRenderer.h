//
// Created by Lucas Wang on 2025-06-08.
//

#ifndef CLUSTEREDDEFERREDRENDERER_DEFERREDRENDERER_H
#define CLUSTEREDDEFERREDRENDERER_DEFERREDRENDERER_H

#include <glad/glad.h>
#include "shader.h"
#include "Scene.h"
#include "camera.h"

struct ClusterAABB {
    glm::vec3 min;
    glm::vec3 max;
};

class DeferredRenderer {
public:
    DeferredRenderer(int width, int height, const Camera& camera);
    ~DeferredRenderer();

    void geometryPass(const Scene& scene, const Camera& camera);
    void lightingPass(const Scene& scene, const Camera& camera);

    int getWidth();
    int getHeight();

    void setScreenSize(int width, int height, const Camera& camera);

private:
    void initGBuffer();

    GLuint gBuffer;
    GLuint gPosition, gNormal, gAlbedoSpec;
    GLuint rboDepth;
    GLuint clusterLightTexture;

    Shader geometryShader;
    Shader lightingShader;

    int screenWidth, screenHeight;
    GLuint quadVAO = 0, quadVBO = 0;

    static const int CLUSTER_X = 16;
    static const int CLUSTER_Y = 9;
    static const int CLUSTER_Z = 24;
    static const int MAX_LIGHTS_PER_CLUSTER = 100;

    std::vector<int> clusterLightCounts;
    std::vector<int> clusterLightIndices; // flattened: CLUSTER_COUNT * MAX_LIGHTS_PER_CLUSTER
    std::vector<ClusterAABB> clusterAABBs;

    void renderQuad();
    void computeClusterBounds(float fov, float aspect, float nearPlane, float farPlane);
    void assignLightsToClusters(const std::vector<Light>& lights, const glm::mat4& viewMatrix);
};

#endif //CLUSTEREDDEFERREDRENDERER_DEFERREDRENDERER_H
