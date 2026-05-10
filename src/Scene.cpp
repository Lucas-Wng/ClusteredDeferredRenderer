//
// Created by Lucas Wang on 2025-06-08.
//

#include <glad/glad.h>
#include "Scene.h"
#include "SceneMath.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/color_space.hpp>
#include <unordered_set>


void Scene::loadModel(const std::string& path) {
    // Release GPU resources from the previous load
    std::unordered_set<GLuint> texIDs;
    for (const Mesh& mesh : meshes) {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
        for (GLuint id : {mesh.diffuseTextureID, mesh.specularGlossinessTextureID,
                          mesh.normalTextureID, mesh.occlusionTextureID, mesh.emissiveTextureID})
            if (id) texIDs.insert(id);
    }
    for (GLuint id : texIDs) glDeleteTextures(1, &id);
    meshes.clear();
    ModelLoader loader;
    meshes = loader.loadModel(path);
    minBounds = loader.minBounds;
    maxBounds = loader.maxBounds;
    normalization = computeSceneNormalizationMatrix(loader.minBounds, loader.maxBounds);

    glm::vec3 basePos = glm::vec3(0.0f, 0.0f, 0.0f);

    int numLights = 8;
    float radius = 30.0f;
    bool rainbow = true;

    if (lights.empty()) {
        for (int i = 0; i < numLights; ++i) {

            float angle = float(i) / float(numLights) * 2.0f * glm::pi<float>();

            glm::vec3 pos = basePos + glm::vec3(cos(angle), 1.0f, sin(angle)) * radius;
            glm::vec3 color;
            if (rainbow) {
                float hue = float(i) / float(numLights);
                color = glm::rgbColor(glm::vec3(hue * 360.0f, 0.8f, 1.0f));
            }
            else {
                color = glm::vec3(1.0f);
            }

            addLight(pos, 50.0f, color, 3.0f);
        }
    }
}

void Scene::drawGeometryPass(const Shader& shader) const {
    shader.use();

    for (const Mesh& mesh : meshes) {
        glm::mat4 model = normalization * mesh.modelMatrix;
        shader.setMat4("model", model);
        shader.setBool("hasNormalMap", mesh.normalTextureID != 0);
        shader.setBool("hasTangents",  mesh.hasTangents);

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
    if (animate) {
        for (size_t i = 0; i < lights.size(); ++i) {
            float angle = time + i;
            float radius = 10.0f;
            lights[i].position.x = radius * std::cos(angle);
            lights[i].position.z = radius * std::sin(angle);
            lights[i].position.y = 2.0f + std::sin(time * 0.5f + i);
        }
    }
}

void Scene::setAnimate(bool on) {
    animate = on;
}

bool Scene::getAnimate() {
    return animate;
}
