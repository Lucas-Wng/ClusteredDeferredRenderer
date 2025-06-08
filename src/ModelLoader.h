#ifndef DEFERREDRENDERER_MODELLOADER_H
#define DEFERREDRENDERER_MODELLOADER_H




#include <glad/glad.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>

struct cgltf_node;
struct cgltf_data;

// represents a single drawable primitive
struct Mesh {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei indexCount;
    glm::mat4 modelMatrix;

    GLuint diffuseTextureID = 0;
    GLuint specularGlossinessTextureID = 0;
    GLuint normalTextureID = 0;
    GLuint occlusionTextureID = 0;
    GLuint emissiveTextureID = 0;
};

class ModelLoader {
public:
    // load and flatten a glTF file into meshes with baked transforms
    std::vector<Mesh> loadModel(const std::string& path);
    glm::vec3 minBounds = glm::vec3(FLT_MAX);
    glm::vec3 maxBounds = glm::vec3(-FLT_MAX);

private:
    void processNode(cgltf_node* node, const glm::mat4& parentTransform, const std::string& directory,
                     std::vector<Mesh>& meshes, const cgltf_data* data);

    GLuint loadTextureFromFile(const std::string& path);
    glm::mat4 getNodeTransform(cgltf_node* node);


};

#endif // DEFERREDRENDERER_MODELLOADER_H
