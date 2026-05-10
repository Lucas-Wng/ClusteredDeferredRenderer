#ifndef CLUSTEREDDEFERREDRENDERER_CLUSTERMATH_H
#define CLUSTEREDDEFERREDRENDERER_CLUSTERMATH_H

#include <algorithm>
#include <cmath>
#include <future>
#include <thread>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct ClusterAABB {
    glm::vec3 min;
    glm::vec3 max;
};

struct ClusterSphereLight {
    glm::vec3 position;
    float radius;
};

struct ClusterLightAssignment {
    std::vector<int> counts;
    std::vector<int> indices;
};

inline int flattenClusterIndex(int x, int y, int z, int clusterX, int clusterY) {
    return x + clusterX * (y + clusterY * z);
}

inline bool sphereIntersectsAABB(const glm::vec3& center, float radius,
                                 const glm::vec3& aabbMin, const glm::vec3& aabbMax) {
    float distSquared = 0.0f;
    for (int i = 0; i < 3; ++i) {
        if (center[i] < aabbMin[i]) {
            const float delta = aabbMin[i] - center[i];
            distSquared += delta * delta;
        } else if (center[i] > aabbMax[i]) {
            const float delta = center[i] - aabbMax[i];
            distSquared += delta * delta;
        }
    }

    return distSquared <= radius * radius;
}

inline std::vector<ClusterAABB> computeClusterAABBs(
        int clusterX, int clusterY, int clusterZ,
        float fovDegrees, float aspect, float nearPlane, float farPlane) {
    std::vector<ClusterAABB> clusterAABBs(clusterX * clusterY * clusterZ);

    const float tanHalfFovY = std::tan(glm::radians(fovDegrees * 0.5f));
    const float tanHalfFovX = tanHalfFovY * aspect;

    for (int z = 0; z < clusterZ; ++z) {
        const float zNear = nearPlane * std::pow(farPlane / nearPlane, static_cast<float>(z) / static_cast<float>(clusterZ));
        const float zFar = nearPlane * std::pow(farPlane / nearPlane, static_cast<float>(z + 1) / static_cast<float>(clusterZ));

        for (int y = 0; y < clusterY; ++y) {
            const float yNearMin = -tanHalfFovY * zNear + 2.0f * tanHalfFovY * zNear * static_cast<float>(y) / static_cast<float>(clusterY);
            const float yNearMax = -tanHalfFovY * zNear + 2.0f * tanHalfFovY * zNear * static_cast<float>(y + 1) / static_cast<float>(clusterY);
            const float yFarMin = -tanHalfFovY * zFar + 2.0f * tanHalfFovY * zFar * static_cast<float>(y) / static_cast<float>(clusterY);
            const float yFarMax = -tanHalfFovY * zFar + 2.0f * tanHalfFovY * zFar * static_cast<float>(y + 1) / static_cast<float>(clusterY);

            for (int x = 0; x < clusterX; ++x) {
                const float xNearMin = -tanHalfFovX * zNear + 2.0f * tanHalfFovX * zNear * static_cast<float>(x) / static_cast<float>(clusterX);
                const float xNearMax = -tanHalfFovX * zNear + 2.0f * tanHalfFovX * zNear * static_cast<float>(x + 1) / static_cast<float>(clusterX);
                const float xFarMin = -tanHalfFovX * zFar + 2.0f * tanHalfFovX * zFar * static_cast<float>(x) / static_cast<float>(clusterX);
                const float xFarMax = -tanHalfFovX * zFar + 2.0f * tanHalfFovX * zFar * static_cast<float>(x + 1) / static_cast<float>(clusterX);

                const int clusterIdx = flattenClusterIndex(x, y, z, clusterX, clusterY);
                clusterAABBs[clusterIdx].min = glm::vec3(
                        std::min({xNearMin, xNearMax, xFarMin, xFarMax}),
                        std::min({yNearMin, yNearMax, yFarMin, yFarMax}),
                        -zFar);
                clusterAABBs[clusterIdx].max = glm::vec3(
                        std::max({xNearMin, xNearMax, xFarMin, xFarMax}),
                        std::max({yNearMin, yNearMax, yFarMin, yFarMax}),
                        -zNear);
            }
        }
    }

    return clusterAABBs;
}

inline ClusterLightAssignment assignSphereLightsToClusters(
        const std::vector<ClusterSphereLight>& lights,
        const glm::mat4& viewMatrix,
        const std::vector<ClusterAABB>& clusterAABBs,
        int maxLightsPerCluster,
        int maxLightsToProcess = 256) {
    const int clusterCount = static_cast<int>(clusterAABBs.size());
    ClusterLightAssignment assignment;
    assignment.counts.assign(clusterCount, 0);
    assignment.indices.assign(clusterCount * maxLightsPerCluster, -1);

    const int lightLimit = std::min(static_cast<int>(lights.size()), maxLightsToProcess);

    // Pre-transform all lights to view space once instead of once per cluster.
    std::vector<glm::vec3> lightViewPositions(lightLimit);
    for (int i = 0; i < lightLimit; ++i)
        lightViewPositions[i] = glm::vec3(viewMatrix * glm::vec4(lights[i].position, 1.0f));

    // Outer loop over clusters: each cluster owns a disjoint slice of indices,
    // so threads write to non-overlapping memory with no synchronisation needed.
    const int nThreads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    const int chunkSize = (clusterCount + nThreads - 1) / nThreads;

    std::vector<std::future<void>> futures;
    futures.reserve(nThreads);

    for (int t = 0; t < nThreads; ++t) {
        const int begin = t * chunkSize;
        const int end   = std::min(begin + chunkSize, clusterCount);
        if (begin >= end) break;

        futures.push_back(std::async(std::launch::async, [&, begin, end]() {
            for (int clusterIdx = begin; clusterIdx < end; ++clusterIdx) {
                const ClusterAABB& aabb = clusterAABBs[clusterIdx];
                int count = 0;
                for (int lightIdx = 0; lightIdx < lightLimit && count < maxLightsPerCluster; ++lightIdx) {
                    if (sphereIntersectsAABB(lightViewPositions[lightIdx], lights[lightIdx].radius, aabb.min, aabb.max)) {
                        assignment.indices[clusterIdx * maxLightsPerCluster + count] = lightIdx;
                        ++count;
                    }
                }
                assignment.counts[clusterIdx] = count;
            }
        }));
    }

    for (auto& f : futures) f.get();
    return assignment;
}

#endif // CLUSTEREDDEFERREDRENDERER_CLUSTERMATH_H
