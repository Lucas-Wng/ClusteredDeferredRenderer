#ifndef CLUSTEREDDEFERREDRENDERER_SCENEMATH_H
#define CLUSTEREDDEFERREDRENDERER_SCENEMATH_H

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

inline glm::mat4 computeSceneNormalizationMatrix(const glm::vec3& minBounds, const glm::vec3& maxBounds) {
    const glm::vec3 center = 0.5f * (minBounds + maxBounds);
    const glm::vec3 bounds = maxBounds - minBounds;
    const float maxExtent = std::max({bounds.x, bounds.y, bounds.z});
    const float scale = maxExtent > 0.0f ? (1.0f / maxExtent) : 1.0f;

    return glm::scale(glm::mat4(1.0f), glm::vec3(scale)) *
           glm::translate(glm::mat4(1.0f), -center);
}

#endif // CLUSTEREDDEFERREDRENDERER_SCENEMATH_H
