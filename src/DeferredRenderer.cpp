//
// Created by Lucas Wang on 2025-06-08.
//

#include "DeferredRenderer.h"
#include "ClusterMath.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

DeferredRenderer::DeferredRenderer(int width, int height, const Camera& camera)
        : screenWidth(width), screenHeight(height), quadVAO(0), quadVBO(0),
          geometryShader("shaders/geometry.vert", "shaders/geometry.frag"),
          lightingShader("shaders/lighting.vert", "shaders/lighting.frag") {
    int numClusters = CLUSTER_X * CLUSTER_Y * CLUSTER_Z;
    clusterLightCounts.resize(numClusters, 0);
    clusterLightIndices.resize(numClusters * MAX_LIGHTS_PER_CLUSTER, -1);

    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float aspect = (float)screenWidth / screenHeight;
    computeClusterBounds(camera.Zoom, aspect, nearPlane, farPlane);

    // Initialize GPU timing queries
    glGenQueries(2, timeQueries);
    
    initGBuffer();
}

DeferredRenderer::~DeferredRenderer() {
    glDeleteFramebuffers(1, &gBuffer);
    glDeleteTextures(1, &gPosition);
    glDeleteTextures(1, &gNormal);
    glDeleteTextures(1, &gAlbedoSpec);
    glDeleteTextures(1, &clusterLightTexture);
    glDeleteRenderbuffers(1, &rboDepth);
    glDeleteQueries(2, timeQueries);

    if (quadVAO != 0) {
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
    }
}

void DeferredRenderer::initGBuffer() {
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete! Status: " << std::hex << status << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &clusterLightTexture);
    glBindTexture(GL_TEXTURE_2D, clusterLightTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<int> emptyData(MAX_LIGHTS_PER_CLUSTER * CLUSTER_X * CLUSTER_Y * CLUSTER_Z, -1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I,
                 MAX_LIGHTS_PER_CLUSTER, CLUSTER_X * CLUSTER_Y * CLUSTER_Z,
                 0, GL_RED_INTEGER, GL_INT, emptyData.data());
}

void DeferredRenderer::geometryPass(const Scene& scene, const Camera& camera) {
    // Start GPU timing
    glBeginQuery(GL_TIME_ELAPSED, timeQueries[currentQueryFrame]);
    
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glViewport(0, 0, screenWidth, screenHeight);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glDisable(GL_BLEND);

    geometryShader.use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / screenHeight, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    geometryShader.setMat4("projection", projection);
    geometryShader.setMat4("view", view);

    scene.drawGeometryPass(geometryShader);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error in geometry pass: 0x" << std::hex << err << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeferredRenderer::lightingPass(const Scene& scene, const Camera& camera) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    lightingShader.use();
    lightingShader.setVec3("viewPos", camera.Position);
    lightingShader.setMat4("view", camera.GetViewMatrix());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    lightingShader.setInt("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    lightingShader.setInt("gNormal", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    lightingShader.setInt("gAlbedoSpec", 2);

    lightingShader.setInt("screenWidth", screenWidth);
    lightingShader.setInt("screenHeight", screenHeight);
    lightingShader.setInt("CLUSTER_X", CLUSTER_X);
    lightingShader.setInt("CLUSTER_Y", CLUSTER_Y);
    lightingShader.setInt("CLUSTER_Z", CLUSTER_Z);
    lightingShader.setInt("MAX_LIGHTS_PER_CLUSTER", MAX_LIGHTS_PER_CLUSTER);
    lightingShader.setFloat("nearPlane", 0.1f);
    lightingShader.setFloat("farPlane", 100.0f);

    const auto& lights = scene.getLights();
    int lightCount = std::min<int>(lights.size(), 256);
    lightingShader.setInt("numLights", lightCount);

    for (int i = 0; i < lightCount; ++i) {
        const Light& light = lights[i];
        lightingShader.setVec4("lights[" + std::to_string(i) + "].position_radius",
                               glm::vec4(light.position, light.radius));
        lightingShader.setVec3("lights[" + std::to_string(i) + "].color", light.color);
        lightingShader.setFloat("lights[" + std::to_string(i) + "].intensity", light.intensity);
    }

    assignLightsToClusters(lights, camera.GetViewMatrix());

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, clusterLightTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    MAX_LIGHTS_PER_CLUSTER, CLUSTER_X * CLUSTER_Y * CLUSTER_Z,
                    GL_RED_INTEGER, GL_INT, clusterLightIndices.data());
    lightingShader.setInt("clusterLightTex", 3);

    renderQuad();

    // End GPU timing and retrieve previous frame's result
    glEndQuery(GL_TIME_ELAPSED);
    
    // Get the result from the previous frame's query
    int prevQueryFrame = 1 - currentQueryFrame;
    GLint available = 0;
    glGetQueryObjectiv(timeQueries[prevQueryFrame], GL_QUERY_RESULT_AVAILABLE, &available);
    if (available) {
        GLuint64 elapsedTime = 0;
        glGetQueryObjectui64v(timeQueries[prevQueryFrame], GL_QUERY_RESULT, &elapsedTime);
        gpuFrameTime = elapsedTime / 1000000.0f; // Convert nanoseconds to milliseconds
    }
    
    // Alternate between two queries
    currentQueryFrame = prevQueryFrame;

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void DeferredRenderer::renderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
                // positions   // texCoords
                -1.0f,  1.0f,  0.0f, 1.0f,
                -1.0f, -1.0f,  0.0f, 0.0f,
                1.0f, -1.0f,  1.0f, 0.0f,

                -1.0f,  1.0f,  0.0f, 1.0f,
                1.0f, -1.0f,  1.0f, 0.0f,
                1.0f,  1.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void DeferredRenderer::computeClusterBounds(float fov, float aspect, float nearPlane, float farPlane) {
    clusterAABBs = computeClusterAABBs(CLUSTER_X, CLUSTER_Y, CLUSTER_Z, fov, aspect, nearPlane, farPlane);
}

void DeferredRenderer::assignLightsToClusters(const std::vector<Light>& lights, const glm::mat4& viewMatrix) {
    std::vector<ClusterSphereLight> sphereLights;
    sphereLights.reserve(lights.size());
    for (const Light& light : lights) {
        sphereLights.push_back({light.position, light.radius});
    }

    ClusterLightAssignment assignment = assignSphereLightsToClusters(
            sphereLights,
            viewMatrix,
            clusterAABBs,
            MAX_LIGHTS_PER_CLUSTER,
            256);

    clusterLightCounts = std::move(assignment.counts);
    clusterLightIndices = std::move(assignment.indices);
}

int DeferredRenderer::getWidth() {
    return screenWidth;
}

int DeferredRenderer::getHeight() {
    return screenHeight;
}

void DeferredRenderer::setScreenSize(int width, int height, const Camera& camera) {
    if (width == 0 || height == 0) return; // avoid divide by zero

    screenWidth = width;
    screenHeight = height;

    initGBuffer();

    float nearPlane = 0.1f;
    float farPlane  = 100.0f;
    float aspect    = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    computeClusterBounds(camera.Zoom, aspect, nearPlane, farPlane);
}
