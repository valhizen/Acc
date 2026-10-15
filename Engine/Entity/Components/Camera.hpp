#pragma once

#include "../ECS.hpp"
#include "Transform.hpp"
#include <glm/glm.hpp>

class CameraComponent : public Component {
private:
  float fieldOfView = 45.0f;
  float aspectRatio = 16.0f / 9.0f;
  float nearPlane = 0.1f;
  float farPlane = 100.0f;

  glm::mat4 viewMatrix = glm::mat4(1.0f);
  glm::mat4 projectionMatrix = glm::mat4(1.0f);

  glm::vec3 target = {0.0f, 0.0f, 0.0f};
  glm::vec3 up = {0.0f, 1.0f, 0.0f};

  bool viewMatrixDirty = true;
  bool projectionMatrixDirty = true;

public:
  explicit CameraComponent(const std::string &componentName = "CameraComponent")
      : Component(componentName) {}

  void Initialize() override;

  void SetFieldOfView(float fov) {
    fieldOfView = fov;
    projectionMatrixDirty = true;
  }

  float GetFieldOfView() const { return fieldOfView; }

  void SetAspectRatio(float ratio) {
    aspectRatio = ratio;
    projectionMatrixDirty = true;
  }

  float GetAspectRation() const { return aspectRatio; }

  void SetClipPlane(float nearPlaneDistance, float farPlaneDistance) {
    nearPlane = nearPlaneDistance;
    farPlane = farPlaneDistance;
    projectionMatrixDirty = true;
  }

  float GetNearPlane() const { return nearPlane; }

  float GetFarPlane() const { return farPlane; }

  void SetTarget(const glm::vec3 &newTarget) {
    target = newTarget;
    viewMatrixDirty = true;
  }

  void SetUp(const glm::vec3 &newUp) {
    up = newUp;
    viewMatrixDirty = true;
  }

  void LookAt(const glm::vec3 &targetPosition,
              const glm::vec3 &upVector = glm::vec3(0.0f, 1.0f, 0.0f)) {
    target = targetPosition;
    up = upVector;
    viewMatrixDirty = true;
  }

  const glm::mat4 &GetViewMatrix();
  const glm::mat4 &GetProjectionMatrix();

  glm::vec3 GetPosition() const {
    auto transform = GetOwner()->GetComponent<TransformComponent>();
    return transform ? transform->GetPosition() : glm::vec3(0.0f, 0.0f, 0.0f);
  }

  const glm::vec3 &GetTarget() const { return target; }

  const glm::vec3 &GetUp() const { return up; }

  void ForceViewMatrixUpdate() { viewMatrixDirty = true; }

private:
  void UpdateViewMatrix();

  void UpdateProjectionMatrix();
};
