#include "SceneMath.h"

#include <gtest/gtest.h>

#include <cmath>

namespace {

constexpr float kEpsilon = 1e-5f;

glm::vec3 transformPoint(const glm::mat4& matrix, const glm::vec3& point) {
    return glm::vec3(matrix * glm::vec4(point, 1.0f));
}

TEST(SceneNormalization, CenterMapsToOriginAndLongestExtentBecomesOne) {
    const glm::vec3 minBounds(-2.0f, -1.0f, -4.0f);
    const glm::vec3 maxBounds(2.0f, 3.0f, 0.0f);

    const glm::mat4 normalization = computeSceneNormalizationMatrix(minBounds, maxBounds);
    const glm::vec3 center = 0.5f * (minBounds + maxBounds);
    const glm::vec3 normalizedCenter = transformPoint(normalization, center);
    const glm::vec3 normalizedMin = transformPoint(normalization, minBounds);
    const glm::vec3 normalizedMax = transformPoint(normalization, maxBounds);

    EXPECT_NEAR(normalizedCenter.x, 0.0f, kEpsilon);
    EXPECT_NEAR(normalizedCenter.y, 0.0f, kEpsilon);
    EXPECT_NEAR(normalizedCenter.z, 0.0f, kEpsilon);

    const glm::vec3 normalizedExtents = normalizedMax - normalizedMin;
    const float longestExtent = std::max({normalizedExtents.x, normalizedExtents.y, normalizedExtents.z});
    EXPECT_NEAR(longestExtent, 1.0f, kEpsilon);
}

TEST(SceneNormalization, UsesUniformScaleAcrossAxes) {
    const glm::vec3 minBounds(-1.0f, -2.0f, -3.0f);
    const glm::vec3 maxBounds(3.0f, 2.0f, 1.0f);

    const glm::mat4 normalization = computeSceneNormalizationMatrix(minBounds, maxBounds);
    const glm::vec3 normalizedMin = transformPoint(normalization, minBounds);
    const glm::vec3 normalizedMax = transformPoint(normalization, maxBounds);
    const glm::vec3 normalizedExtents = normalizedMax - normalizedMin;

    EXPECT_NEAR(normalizedExtents.x, normalizedExtents.y, kEpsilon);
    EXPECT_NEAR(normalizedExtents.y, normalizedExtents.z, kEpsilon);
}

TEST(SceneNormalization, DegenerateBoundsStayFiniteAndCentered) {
    const glm::vec3 bounds(5.0f, -2.0f, 7.0f);
    const glm::mat4 normalization = computeSceneNormalizationMatrix(bounds, bounds);

    const glm::vec3 normalizedCenter = transformPoint(normalization, bounds);
    EXPECT_NEAR(normalizedCenter.x, 0.0f, kEpsilon);
    EXPECT_NEAR(normalizedCenter.y, 0.0f, kEpsilon);
    EXPECT_NEAR(normalizedCenter.z, 0.0f, kEpsilon);

    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            EXPECT_TRUE(std::isfinite(normalization[col][row]));
        }
    }
}

} // namespace
