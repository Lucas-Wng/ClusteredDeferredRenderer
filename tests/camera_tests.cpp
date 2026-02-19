#include "camera.h"

#include <gtest/gtest.h>

namespace {

constexpr float kEpsilon = 1e-4f;

void expectVec3Near(const glm::vec3& actual, const glm::vec3& expected, float epsilon = kEpsilon) {
    EXPECT_NEAR(actual.x, expected.x, epsilon);
    EXPECT_NEAR(actual.y, expected.y, epsilon);
    EXPECT_NEAR(actual.z, expected.z, epsilon);
}

TEST(CameraDefaults, BasisVectorsMatchExpectedAxes) {
    Camera camera;
    expectVec3Near(camera.Front, glm::vec3(0.0f, 0.0f, -1.0f));
    expectVec3Near(camera.Up, glm::vec3(0.0f, 1.0f, 0.0f));
    expectVec3Near(camera.Right, glm::vec3(1.0f, 0.0f, 0.0f));
}

TEST(CameraDefaults, ScalarDefaultsMatchConstants) {
    Camera camera;
    EXPECT_FLOAT_EQ(camera.Yaw, YAW);
    EXPECT_FLOAT_EQ(camera.Pitch, PITCH);
    EXPECT_FLOAT_EQ(camera.MovementSpeed, SPEED);
    EXPECT_FLOAT_EQ(camera.MouseSensitivity, SENSITIVITY);
    EXPECT_FLOAT_EQ(camera.Zoom, ZOOM);
}

TEST(CameraMovement, ForwardAndBackwardCancel) {
    Camera camera;

    camera.ProcessKeyboard(FORWARD, 2.0f);
    expectVec3Near(camera.Position, glm::vec3(0.0f, 0.0f, -5.0f));

    camera.ProcessKeyboard(BACKWARD, 2.0f);
    expectVec3Near(camera.Position, glm::vec3(0.0f, 0.0f, 0.0f));
}

TEST(CameraMovement, RightAndLeftCancel) {
    Camera camera;

    camera.ProcessKeyboard(RIGHT, 1.0f);
    expectVec3Near(camera.Position, glm::vec3(2.5f, 0.0f, 0.0f));

    camera.ProcessKeyboard(LEFT, 1.0f);
    expectVec3Near(camera.Position, glm::vec3(0.0f, 0.0f, 0.0f));
}

TEST(CameraMovement, UpAndDownCancel) {
    Camera camera;

    camera.ProcessKeyboard(UP, 1.0f);
    expectVec3Near(camera.Position, glm::vec3(0.0f, 2.5f, 0.0f));

    camera.ProcessKeyboard(DOWN, 1.0f);
    expectVec3Near(camera.Position, glm::vec3(0.0f, 0.0f, 0.0f));
}

TEST(CameraMouse, MovementUpdatesAnglesWithSensitivity) {
    Camera camera;
    camera.ProcessMouseMovement(90.0f, 20.0f);

    EXPECT_NEAR(camera.Yaw, -81.0f, kEpsilon);
    EXPECT_NEAR(camera.Pitch, 2.0f, kEpsilon);
}

TEST(CameraMouse, OrientationVectorsRemainOrthonormal) {
    Camera camera;
    camera.ProcessMouseMovement(90.0f, 20.0f);

    EXPECT_NEAR(glm::length(camera.Front), 1.0f, kEpsilon);
    EXPECT_NEAR(glm::length(camera.Up), 1.0f, kEpsilon);
    EXPECT_NEAR(glm::length(camera.Right), 1.0f, kEpsilon);
    EXPECT_NEAR(glm::dot(camera.Front, camera.Up), 0.0f, 1e-3f);
    EXPECT_NEAR(glm::dot(camera.Front, camera.Right), 0.0f, 1e-3f);
    EXPECT_NEAR(glm::dot(camera.Up, camera.Right), 0.0f, 1e-3f);
}

TEST(CameraMouse, PitchIsClampedByDefault) {
    Camera camera;

    camera.ProcessMouseMovement(0.0f, 2000.0f);
    EXPECT_NEAR(camera.Pitch, 89.0f, kEpsilon);

    camera.ProcessMouseMovement(0.0f, -4000.0f);
    EXPECT_NEAR(camera.Pitch, -89.0f, kEpsilon);
}

TEST(CameraMouse, PitchCanExceedClampWhenDisabled) {
    Camera camera;
    camera.ProcessMouseMovement(0.0f, 2000.0f, false);
    EXPECT_NEAR(camera.Pitch, 200.0f, kEpsilon);
}

TEST(CameraZoom, ScrollIsClampedToRange) {
    Camera camera;

    camera.ProcessMouseScroll(10.0f);
    EXPECT_NEAR(camera.Zoom, 35.0f, kEpsilon);

    camera.ProcessMouseScroll(100.0f);
    EXPECT_NEAR(camera.Zoom, 1.0f, kEpsilon);

    camera.ProcessMouseScroll(-100.0f);
    EXPECT_NEAR(camera.Zoom, 45.0f, kEpsilon);
}

TEST(CameraViewMatrix, MatchesLookAtComputation) {
    Camera camera(glm::vec3(1.0f, 2.0f, 3.0f));
    camera.ProcessMouseMovement(45.0f, -10.0f);

    const glm::mat4 expected = glm::lookAt(camera.Position, camera.Position + camera.Front, camera.Up);
    const glm::mat4 actual = camera.GetViewMatrix();

    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            EXPECT_NEAR(actual[col][row], expected[col][row], 1e-5f);
        }
    }
}

} // namespace
