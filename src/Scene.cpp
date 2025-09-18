//
// Created by Lucas Wang on 2025-06-08.
//

#include <glad/glad.h>
#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>


void Scene::loadModel(const std::string& path) {
    ModelLoader loader;
    meshes = loader.loadModel(path);
    minBounds = loader.minBounds;
    maxBounds = loader.maxBounds;
    glm::vec3 center = 0.5f * (loader.minBounds + loader.maxBounds);
    glm::vec3 bounds = loader.maxBounds - loader.minBounds;
    float maxExtent = std::max({ bounds.x, bounds.y, bounds.z });
    
    // Debug output to see model bounds
    std::cout << "Model bounds: min(" << loader.minBounds.x << ", " << loader.minBounds.y << ", " << loader.minBounds.z << ") "
              << "max(" << loader.maxBounds.x << ", " << loader.maxBounds.y << ", " << loader.maxBounds.z << ")" << std::endl;
    std::cout << "Max extent: " << maxExtent << std::endl;
    std::cout << "Center: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
    
    // No scaling - just center the model
    float scale = 1.0f / maxExtent;
    normalization = glm::scale(glm::mat4(1.0f), glm::vec3(scale)) *
                    glm::translate(glm::mat4(1.0f), -center);

    // evenly distributed test lights around the model
    glm::vec3 basePos = glm::vec3(0.0f, 0.0f, 0.0f);

    int numLights = 8;
    float radius = 30.0f; // Radius to match unscaled model size
    bool rainbow = true;

    for (int i = 0; i < numLights; ++i) {

        float angle = float(i) / float(numLights) * 2.0f * glm::pi<float>();

        // Position in a circle, slightly elevated
        glm::vec3 pos = basePos + glm::vec3(cos(angle), 1.0f, sin(angle)) * radius;
        glm::vec3 color;
        if (rainbow) {
            // Rainbow color via HSV → RGB
            float hue = float(i) / float(numLights);  // Range [0,1]
            color = glm::rgbColor(glm::vec3(hue * 360.0f, 0.8f, 1.0f));  // glm::rgbColor takes HSV in degrees
        }
        else {
            color = glm::vec3(1.0f);
        }

        addLight(pos, 50.0f, color, 3.0f);
    }
}

void Scene::drawGeometryPass(const Shader& shader) const {
    shader.use();

    for (const Mesh& mesh : meshes) {
        // Apply both the mesh's local transform and normalization
        glm::mat4 model = normalization * mesh.modelMatrix;
        shader.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, mesh.diffuseTextureID);
        shader.setInt("diffuseTexture", 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, mesh.specularGlossinessTextureID);
        shader.setInt("specularGlossinessTexture", 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, mesh.normalTextureID);
        shader.setInt("normalTexture", 2);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, mesh.occlusionTextureID);
        shader.setInt("occlusionTexture", 3);
        glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, mesh.emissiveTextureID);
        shader.setInt("emissiveTexture", 4);

        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
    }
}

const std::vector<Light>& Scene::getLights() const {
    return lights;
}

void Scene::addLight(const glm::vec3& position, float radius, const glm::vec3& color, float intensity) {
    lights.push_back({position, radius, color, intensity});
}

void Scene::updateLights(float time) {
    for (size_t i = 0; i < lights.size(); ++i) {
        float angle = time + i;  // offset each light
        float radius = 10.0f;
        lights[i].position.x = radius * std::cos(angle);
        lights[i].position.z = radius * std::sin(angle);
        // Optionally animate y too
        lights[i].position.y = 2.0f + std::sin(time * 0.5f + i);
    }
}
