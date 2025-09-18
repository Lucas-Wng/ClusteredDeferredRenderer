//
// Created by Lucas Wang on 2025-06-08.
//

#ifndef CLUSTEREDDEFERREDRENDERER_SCENE_H
#define CLUSTEREDDEFERREDRENDERER_SCENE_H


#include "shader.h"
#include "camera.h"
#include "ModelLoader.h"
#include <vector>
#include <glm/glm.hpp>

struct Light {
    glm::vec3 position;  // world space
    float radius;
    glm::vec3 color;     // light color
    float intensity;     // light intensity
};

class Scene {
public:
    void loadModel(const std::string& path);
    void drawGeometryPass(const Shader& shader) const;
    const std::vector<Light>& getLights() const;
    void addLight(const glm::vec3& position, float radius, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);
    void updateLights(float time);

    glm::vec3 minBounds;
    glm::vec3 maxBounds;

private:
    std::vector<Mesh> meshes;
    glm::mat4 normalization;
    std::vector<Light> lights;
};


#endif //CLUSTEREDDEFERREDRENDERER_SCENE_H
