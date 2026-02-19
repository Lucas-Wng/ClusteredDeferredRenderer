#include "ClusterMath.h"

#include <gtest/gtest.h>

namespace {

constexpr float kEpsilon = 1e-5f;

TEST(SphereAabbIntersection, SphereInsideBoxIntersects) {
    const glm::vec3 boxMin(-1.0f, -1.0f, -1.0f);
    const glm::vec3 boxMax(1.0f, 1.0f, 1.0f);

    EXPECT_TRUE(sphereIntersectsAABB(glm::vec3(0.0f), 0.25f, boxMin, boxMax));
}

TEST(SphereAabbIntersection, SphereTouchingBoxBoundaryIntersects) {
    const glm::vec3 boxMin(-1.0f, -1.0f, -1.0f);
    const glm::vec3 boxMax(1.0f, 1.0f, 1.0f);

    EXPECT_TRUE(sphereIntersectsAABB(glm::vec3(2.0f, 0.0f, 0.0f), 1.0f, boxMin, boxMax));
}

TEST(SphereAabbIntersection, SphereOutsideBoxDoesNotIntersect) {
    const glm::vec3 boxMin(-1.0f, -1.0f, -1.0f);
    const glm::vec3 boxMax(1.0f, 1.0f, 1.0f);

    EXPECT_FALSE(sphereIntersectsAABB(glm::vec3(2.2f, 0.0f, 0.0f), 1.0f, boxMin, boxMax));
}

TEST(FrustumSlicing, ProducesExpectedDepthSlices) {
    const std::vector<ClusterAABB> aabbs = computeClusterAABBs(2, 2, 2, 90.0f, 1.0f, 1.0f, 9.0f);
    ASSERT_EQ(aabbs.size(), 8);

    const ClusterAABB& nearSlice = aabbs[flattenClusterIndex(0, 0, 0, 2, 2)];
    const ClusterAABB& farSlice = aabbs[flattenClusterIndex(0, 0, 1, 2, 2)];

    EXPECT_NEAR(nearSlice.max.z, -1.0f, kEpsilon);
    EXPECT_NEAR(nearSlice.min.z, -3.0f, kEpsilon);
    EXPECT_NEAR(farSlice.max.z, -3.0f, kEpsilon);
    EXPECT_NEAR(farSlice.min.z, -9.0f, kEpsilon);
    EXPECT_NEAR(nearSlice.min.z, farSlice.max.z, kEpsilon);
}

TEST(FrustumSlicing, ClusterExtentGrowsWithDepth) {
    const std::vector<ClusterAABB> aabbs = computeClusterAABBs(1, 1, 2, 90.0f, 1.0f, 1.0f, 9.0f);
    ASSERT_EQ(aabbs.size(), 2);

    const ClusterAABB& nearSlice = aabbs[0];
    const ClusterAABB& farSlice = aabbs[1];

    const float nearWidth = nearSlice.max.x - nearSlice.min.x;
    const float farWidth = farSlice.max.x - farSlice.min.x;
    const float nearHeight = nearSlice.max.y - nearSlice.min.y;
    const float farHeight = farSlice.max.y - farSlice.min.y;

    EXPECT_GT(farWidth, nearWidth);
    EXPECT_GT(farHeight, nearHeight);
}

TEST(LightClustering, AssignsLightsToExpectedClusters) {
    std::vector<ClusterAABB> aabbs(2);
    aabbs[0] = {glm::vec3(-1.0f, -1.0f, -2.0f), glm::vec3(1.0f, 1.0f, -1.0f)};
    aabbs[1] = {glm::vec3(-1.0f, -1.0f, -4.0f), glm::vec3(1.0f, 1.0f, -2.0f)};

    const std::vector<ClusterSphereLight> lights = {
            {glm::vec3(0.0f, 0.0f, -1.5f), 0.2f},  // near cluster only
            {glm::vec3(0.0f, 0.0f, -3.0f), 0.2f},  // far cluster only
            {glm::vec3(0.0f, 0.0f, -2.0f), 0.01f}  // on shared boundary
    };

    const ClusterLightAssignment assignment = assignSphereLightsToClusters(
            lights, glm::mat4(1.0f), aabbs, 4);

    ASSERT_EQ(assignment.counts.size(), 2);
    ASSERT_EQ(assignment.indices.size(), 8);

    EXPECT_EQ(assignment.counts[0], 2);
    EXPECT_EQ(assignment.indices[0], 0);
    EXPECT_EQ(assignment.indices[1], 2);

    EXPECT_EQ(assignment.counts[1], 2);
    EXPECT_EQ(assignment.indices[4], 1);
    EXPECT_EQ(assignment.indices[5], 2);
}

TEST(LightClustering, RespectsPerClusterCapacity) {
    const std::vector<ClusterAABB> aabbs = {
            {glm::vec3(-1.0f), glm::vec3(1.0f)}
    };

    const std::vector<ClusterSphereLight> lights = {
            {glm::vec3(0.0f), 0.2f},
            {glm::vec3(0.1f), 0.2f},
            {glm::vec3(0.2f), 0.2f},
            {glm::vec3(0.3f), 0.2f}
    };

    const ClusterLightAssignment assignment = assignSphereLightsToClusters(
            lights, glm::mat4(1.0f), aabbs, 3);

    EXPECT_EQ(assignment.counts[0], 3);
    EXPECT_EQ(assignment.indices[0], 0);
    EXPECT_EQ(assignment.indices[1], 1);
    EXPECT_EQ(assignment.indices[2], 2);
}

TEST(LightClustering, RespectsGlobalLightProcessingLimit) {
    const std::vector<ClusterAABB> aabbs = {
            {glm::vec3(-1.0f), glm::vec3(1.0f)}
    };

    std::vector<ClusterSphereLight> lights;
    lights.reserve(300);
    for (int i = 0; i < 300; ++i) {
        lights.push_back({glm::vec3(0.0f), 0.2f});
    }

    const ClusterLightAssignment assignment = assignSphereLightsToClusters(
            lights, glm::mat4(1.0f), aabbs, 300);

    EXPECT_EQ(assignment.counts[0], 256);
    EXPECT_EQ(assignment.indices[0], 0);
    EXPECT_EQ(assignment.indices[255], 255);
    EXPECT_EQ(assignment.indices[256], -1);
}

} // namespace
