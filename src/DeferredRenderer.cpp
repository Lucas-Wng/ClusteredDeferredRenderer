//
// Created by Lucas Wang on 2025-06-08.
//

#include "DeferredRenderer.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

bool sphereIntersectsAABB(const glm::vec3& center, float radius, const glm::vec3& aabbMin, const glm::vec3& aabbMax) {
    float distSquared = 0.0f;
    for (int i = 0; i < 3; ++i) {
        if (center[i] < aabbMin[i]) {
            distSquared += (aabbMin[i] - center[i]) * (aabbMin[i] - center[i]);
        } else if (center[i] > aabbMax[i]) {
            distSquared += (center[i] - aabbMax[i]) * (center[i] - aabbMax[i]);
        }
    }

    return distSquared <= radius * radius;
}

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

    initGBuffer();
}

DeferredRenderer::~DeferredRenderer() {
    glDeleteFramebuffers(1, &gBuffer);
    glDeleteTextures(1, &gPosition);
    glDeleteTextures(1, &gNormal);
    glDeleteTextures(1, &gAlbedoSpec);
    glDeleteTextures(1, &clusterLightTexture);
    glDeleteRenderbuffers(1, &rboDepth);

    if (quadVAO != 0) {
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
    }
}

void DeferredRenderer::initGBuffer() {
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // Position buffer (RGB16F for higher precision)
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // Normal buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // Albedo + specular buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    // Specify which color attachments to use
    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    // Depth buffer
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete! Status: " << std::hex << status << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Initialize cluster light texture
    glGenTextures(1, &clusterLightTexture);
    glBindTexture(GL_TEXTURE_2D, clusterLightTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Initialize with empty data
    std::vector<int> emptyData(MAX_LIGHTS_PER_CLUSTER * CLUSTER_X * CLUSTER_Y * CLUSTER_Z, -1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I,
                 MAX_LIGHTS_PER_CLUSTER, CLUSTER_X * CLUSTER_Y * CLUSTER_Z,
                 0, GL_RED_INTEGER, GL_INT, emptyData.data());
}

void DeferredRenderer::geometryPass(const Scene& scene, const Camera& camera) {
    // Bind G-buffer and clear
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glViewport(0, 0, screenWidth, screenHeight);

    // Clear all color attachments and depth
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Disable blending for geometry pass
    glDisable(GL_BLEND);

    geometryShader.use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / screenHeight, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    geometryShader.setMat4("projection", projection);
    geometryShader.setMat4("view", view);

    scene.drawGeometryPass(geometryShader);

    // Check for OpenGL errors
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error in geometry pass: 0x" << std::hex << err << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeferredRenderer::lightingPass(const Scene& scene, const Camera& camera) {
    // Bind default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Disable depth testing for fullscreen quad
    glDisable(GL_DEPTH_TEST);

    // Enable additive blending for multiple lights
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    lightingShader.use();
    lightingShader.setVec3("viewPos", camera.Position);
    lightingShader.setMat4("view", camera.GetViewMatrix());

    // Bind G-buffer textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    lightingShader.setInt("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    lightingShader.setInt("gNormal", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    lightingShader.setInt("gAlbedoSpec", 2);

    // Set clustering parameters
    lightingShader.setInt("screenWidth", screenWidth);
    lightingShader.setInt("screenHeight", screenHeight);
    lightingShader.setInt("CLUSTER_X", CLUSTER_X);
    lightingShader.setInt("CLUSTER_Y", CLUSTER_Y);
    lightingShader.setInt("CLUSTER_Z", CLUSTER_Z);
    lightingShader.setInt("MAX_LIGHTS_PER_CLUSTER", MAX_LIGHTS_PER_CLUSTER);
    lightingShader.setFloat("nearPlane", 0.1f);
    lightingShader.setFloat("farPlane", 100.0f);

    // Set up lights
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

    // Update cluster-light assignments
    assignLightsToClusters(lights, camera.GetViewMatrix());

    // Update cluster light texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, clusterLightTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    MAX_LIGHTS_PER_CLUSTER, CLUSTER_X * CLUSTER_Y * CLUSTER_Z,
                    GL_RED_INTEGER, GL_INT, clusterLightIndices.data());
    lightingShader.setInt("clusterLightTex", 3);

    renderQuad();

    // Reset blend state
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
    clusterAABBs.clear();
    clusterAABBs.resize(CLUSTER_X * CLUSTER_Y * CLUSTER_Z);

    float tanHalfFovY = tan(glm::radians(fov / 2.0f));
    float tanHalfFovX = tanHalfFovY * aspect;

    for (int z = 0; z < CLUSTER_Z; ++z) {
        // Exponential depth slicing for better distribution
        float zNear = nearPlane * std::pow(farPlane / nearPlane, float(z) / CLUSTER_Z);
        float zFar  = nearPlane * std::pow(farPlane / nearPlane, float(z + 1) / CLUSTER_Z);

        for (int y = 0; y < CLUSTER_Y; ++y) {
            // Calculate Y bounds in view space
            float yNearMin = -tanHalfFovY * zNear + 2.0f * tanHalfFovY * zNear * float(y) / CLUSTER_Y;
            float yNearMax = -tanHalfFovY * zNear + 2.0f * tanHalfFovY * zNear * float(y + 1) / CLUSTER_Y;

            float yFarMin = -tanHalfFovY * zFar + 2.0f * tanHalfFovY * zFar * float(y) / CLUSTER_Y;
            float yFarMax = -tanHalfFovY * zFar + 2.0f * tanHalfFovY * zFar * float(y + 1) / CLUSTER_Y;

            for (int x = 0; x < CLUSTER_X; ++x) {
                // Calculate X bounds in view space
                float xNearMin = -tanHalfFovX * zNear + 2.0f * tanHalfFovX * zNear * float(x) / CLUSTER_X;
                float xNearMax = -tanHalfFovX * zNear + 2.0f * tanHalfFovX * zNear * float(x + 1) / CLUSTER_X;

                float xFarMin = -tanHalfFovX * zFar + 2.0f * tanHalfFovX * zFar * float(x) / CLUSTER_X;
                float xFarMax = -tanHalfFovX * zFar + 2.0f * tanHalfFovX * zFar * float(x + 1) / CLUSTER_X;

                int clusterIdx = x + CLUSTER_X * (y + CLUSTER_Y * z);

                // Create AABB that encompasses the frustum slice
                clusterAABBs[clusterIdx].min = glm::vec3(
                        std::min({xNearMin, xNearMax, xFarMin, xFarMax}),
                        std::min({yNearMin, yNearMax, yFarMin, yFarMax}),
                        -zFar  // Negative because view space Z points toward viewer
                );
                clusterAABBs[clusterIdx].max = glm::vec3(
                        std::max({xNearMin, xNearMax, xFarMin, xFarMax}),
                        std::max({yNearMin, yNearMax, yFarMin, yFarMax}),
                        -zNear
                );
            }
        }
    }
}

void DeferredRenderer::assignLightsToClusters(const std::vector<Light>& lights, const glm::mat4& viewMatrix) {
    // Clear cluster light data
    std::fill(clusterLightCounts.begin(), clusterLightCounts.end(), 0);
    std::fill(clusterLightIndices.begin(), clusterLightIndices.end(), -1);

    for (int lightIdx = 0; lightIdx < lights.size() && lightIdx < 256; ++lightIdx) {
        const Light& light = lights[lightIdx];
        glm::vec3 lightViewPos = glm::vec3(viewMatrix * glm::vec4(light.position, 1.0f));

        for (int clusterIdx = 0; clusterIdx < clusterAABBs.size(); ++clusterIdx) {
            const ClusterAABB& aabb = clusterAABBs[clusterIdx];

            if (sphereIntersectsAABB(lightViewPos, light.radius, aabb.min, aabb.max)) {
                int count = clusterLightCounts[clusterIdx];
                if (count < MAX_LIGHTS_PER_CLUSTER) {
                    clusterLightIndices[clusterIdx * MAX_LIGHTS_PER_CLUSTER + count] = lightIdx;
                    clusterLightCounts[clusterIdx]++;
                }
            }
        }
    }
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

    // Rebuild G-buffer attachments for new resolution
    initGBuffer();

    // Recompute cluster bounds with new aspect ratio
    float nearPlane = 0.1f;
    float farPlane  = 100.0f;
    float aspect    = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    computeClusterBounds(camera.Zoom, aspect, nearPlane, farPlane);
}
