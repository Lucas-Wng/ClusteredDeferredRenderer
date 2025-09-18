#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "ModelLoader.h"
#include <iostream>
#include <filesystem>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>


GLuint ModelLoader::loadTextureFromFile(const std::string& path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << "\n";
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    
    // Use sRGB format for diffuse textures (base color)
    // Check if this is a diffuse/base color texture by filename
    bool isDiffuse = path.find("baseColor") != std::string::npos || 
                     path.find("diffuse") != std::string::npos ||
                     path.find("albedo") != std::string::npos;
    
    GLint internalFormat = isDiffuse ? GL_SRGB8_ALPHA8 : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return texID;
}

glm::mat4 ModelLoader::getNodeTransform(cgltf_node* node) {
    glm::mat4 mat(1.0f);
    if (node->has_matrix) {
        memcpy(glm::value_ptr(mat), node->matrix, sizeof(float) * 16);
    } else {
        glm::vec3 translation(0.0f), scale(1.0f);
        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
        if (node->has_translation)
            translation = glm::make_vec3(node->translation);
        if (node->has_rotation) {
            float* r = node->rotation;
            rotation = glm::quat(r[3], r[0], r[1], r[2]);  // glTF: x, y, z, w → GLM: w, x, y, z
        }
        if (node->has_scale)
            scale = glm::make_vec3(node->scale);
        mat = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
    }
    return mat;
}

void ModelLoader::processNode(cgltf_node* node, const glm::mat4& parentTransform, const std::string& directory,
                              std::vector<Mesh>& meshes, const cgltf_data* data) {
    static std::unordered_map<std::string, GLuint> textureCache;  // Caches texture loads

    glm::mat4 localTransform = getNodeTransform(node);
    glm::mat4 transform = parentTransform * localTransform;

    if (node->mesh) {
        for (cgltf_size p = 0; p < node->mesh->primitives_count; ++p) {
            cgltf_primitive* prim = &node->mesh->primitives[p];

            cgltf_accessor* posAccessor = nullptr;
            cgltf_accessor* normAccessor = nullptr;
            cgltf_accessor* uvAccessor = nullptr;
            cgltf_accessor* tangentAccessor = nullptr;

            for (cgltf_size i = 0; i < prim->attributes_count; ++i) {
                cgltf_attribute* attr = &prim->attributes[i];
                if (strcmp(attr->name, "POSITION") == 0) posAccessor = attr->data;
                if (strcmp(attr->name, "NORMAL") == 0) normAccessor = attr->data;
                if (strcmp(attr->name, "TEXCOORD_0") == 0) uvAccessor = attr->data;
                if (strcmp(attr->name, "TANGENT") == 0) tangentAccessor = attr->data;
            }

            if (!posAccessor) {
                std::cerr << "Missing POSITION accessor. Skipping primitive.\n";
                continue;
            }

            size_t count = posAccessor->count;
            std::vector<float> vertices(count * 12);  // 3 + 3 + 2 + 4 floats per vertex

            for (size_t i = 0; i < count; ++i) {
                float pos[3], norm[3] = {0}, uv[2] = {0}, tangent[4] = {0,0,0,1};
                cgltf_accessor_read_float(posAccessor, i, pos, 3);
                if (normAccessor) cgltf_accessor_read_float(normAccessor, i, norm, 3);
                if (uvAccessor) cgltf_accessor_read_float(uvAccessor, i, uv, 2);
                if (tangentAccessor) cgltf_accessor_read_float(tangentAccessor, i, tangent, 4);

                // Store vertices in model space (let Scene handle all transformations)
                glm::vec3 modelPos = glm::vec3(pos[0], pos[1], pos[2]);
                glm::vec3 modelNormal = glm::vec3(norm[0], norm[1], norm[2]);
                glm::vec3 modelTangent = glm::vec3(tangent[0], tangent[1], tangent[2]);

                // Update bounds using model space position for correct centering
                minBounds = glm::min(minBounds, modelPos);
                maxBounds = glm::max(maxBounds, modelPos);

                size_t offset = i * 12;
                vertices[offset + 0] = modelPos.x;
                vertices[offset + 1] = modelPos.y;
                vertices[offset + 2] = modelPos.z;
                vertices[offset + 3] = modelNormal.x;
                vertices[offset + 4] = modelNormal.y;
                vertices[offset + 5] = modelNormal.z;
                vertices[offset + 6] = uv[0];
                vertices[offset + 7] = uv[1];
                vertices[offset + 8] = modelTangent.x;
                vertices[offset + 9] = modelTangent.y;
                vertices[offset +10] = modelTangent.z;
                vertices[offset +11] = tangent[3];  // keep handedness as-is
            }

            std::vector<unsigned int> indices;
            if (prim->indices) {
                size_t index_count = prim->indices->count;
                indices.resize(index_count);
                for (size_t i = 0; i < index_count; ++i) {
                    indices[i] = static_cast<unsigned int>(cgltf_accessor_read_index(prim->indices, i));
                }
            }

            GLuint vao, vbo, ebo;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            GLsizei stride = 12 * sizeof(float);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
            glEnableVertexAttribArray(3);
            glBindVertexArray(0);

            GLuint diffuseTex = 0, specGlossTex = 0, normalTex = 0, occlusionTex = 0, emissiveTex = 0;

            auto loadTex = [&](cgltf_texture_view view) -> GLuint {
                if (view.texture && view.texture->image && view.texture->image->uri) {
                    std::string texPath = directory + "/" + view.texture->image->uri;
                    if (textureCache.count(texPath)) return textureCache[texPath];
                    GLuint id = loadTextureFromFile(texPath);
                    textureCache[texPath] = id;
                    return id;
                }
                return 0;
            };

            if (prim->material) {
                cgltf_material* mat = prim->material;
                if (mat->has_pbr_specular_glossiness) {
                    diffuseTex = loadTex(mat->pbr_specular_glossiness.diffuse_texture);
                    specGlossTex = loadTex(mat->pbr_specular_glossiness.specular_glossiness_texture);
                } else if (mat->has_pbr_metallic_roughness) {
                    diffuseTex = loadTex(mat->pbr_metallic_roughness.base_color_texture);
                }
                normalTex = loadTex(mat->normal_texture);
                occlusionTex = loadTex(mat->occlusion_texture);
                emissiveTex = loadTex(mat->emissive_texture);
            }

            meshes.push_back(Mesh{
                    vao, vbo, ebo,
                    static_cast<GLsizei>(indices.size()),
                    transform,
                    diffuseTex, specGlossTex, normalTex, occlusionTex, emissiveTex
            });
        }
    }

    for (cgltf_size i = 0; i < node->children_count; ++i) {
        processNode(node->children[i], transform, directory, meshes, data);
    }
}

std::vector<Mesh> ModelLoader::loadModel(const std::string& path) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success ||
        cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success ||
        cgltf_validate(data) != cgltf_result_success) {
        std::cerr << "Failed to load glTF: " << path << "\n";
        if (data) cgltf_free(data);
        return {};
    }

    std::vector<Mesh> meshes;
    std::string directory = std::filesystem::path(path).parent_path().string();

    // Reset bounds before processing
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    if (data->scene) {
        for (cgltf_size i = 0; i < data->scene->nodes_count; ++i) {
            processNode(data->scene->nodes[i], glm::mat4(1.0f), directory, meshes, data);
        }
    } else {
        std::cerr << "No default scene found in glTF file.\n";
    }

    cgltf_free(data);

    // === Normalize the model ===
    if (!meshes.empty()) {
        glm::vec3 size   = maxBounds - minBounds;
        float maxExtent  = glm::max(glm::max(size.x, size.y), size.z);
        glm::vec3 center = (maxBounds + minBounds) * 0.5f;

        float scale = 1.0f / maxExtent; // fits into unit cube
        glm::mat4 normalize =
                glm::scale(glm::mat4(1.0f), glm::vec3(scale)) *
                glm::translate(glm::mat4(1.0f), -center);

        for (auto& mesh : meshes) {
            mesh.modelMatrix = normalize * mesh.modelMatrix;
        }
    }

    return meshes;
}
