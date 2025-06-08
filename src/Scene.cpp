//
// Created by Lucas Wang on 2025-06-08.
//

#include <glad/glad.h>
#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>


void Scene::loadModel(const std::string& path) {
    ModelLoader loader;
    meshes = loader.loadModel(path);
    glm::vec3 center = 0.5f * (loader.minBounds + loader.maxBounds);
    glm::vec3 bounds = loader.maxBounds - loader.minBounds;
    float maxExtent = std::max({ bounds.x, bounds.y, bounds.z });
    float scale = 10.0f / maxExtent;

    normalization = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    glm::mat4 axisCorrection = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    normalization = axisCorrection * normalization;
    normalization = glm::translate(normalization, -center);
}

void Scene::drawGeometryPass(const Shader& shader) const {
    shader.use();

    for (const Mesh& mesh : meshes) {
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