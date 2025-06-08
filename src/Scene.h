//
// Created by Lucas Wang on 2025-06-08.
//

#ifndef CLUSTEREDDEFERREDRENDERER_SCENE_H
#define CLUSTEREDDEFERREDRENDERER_SCENE_H


#include "shader.h"
#include "camera.h"
#include "ModelLoader.h"
#include <vector>

class Scene {
public:
    void loadModel(const std::string& path);
    void drawGeometryPass(const Shader& shader) const;

private:
    std::vector<Mesh> meshes;
    glm::mat4 normalization;
};


#endif //CLUSTEREDDEFERREDRENDERER_SCENE_H
