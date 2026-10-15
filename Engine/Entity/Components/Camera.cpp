#include "Camera.hpp"


void CameraComponent::Initialize() {
  UpdateViewMatrix();
  UpdateProjectionMatrix();
}
const glm::mat4 &CameraComponent::GetViewMatrix() {
  if (viewMatrixDirty) {
    UpdateViewMatrix();
  }
  return viewMatrix;
}

const glm::mat4 &CameraComponent::GetProjectionMatrix() {
  if (projectionMatrixDirty) {
    UpdateProjectionMatrix();
  }
  return projectionMatrix;
}

void CameraComponent::UpdateViewMatrix() {
  auto transformComponent = owner->GetComponent<TransformComponent>();
  if (transformComponent) {
    // Build camera world transform (T * R) from the camera entity's transform
    // and compute the view matrix as its inverse. This ensures consistency
    // with rasterization and avoids relying on an external target vector.
    const glm::vec3 position = transformComponent->GetPosition();
    const glm::vec3 euler = transformComponent->GetRotation(); // radians

    const glm::quat qx = glm::angleAxis(euler.x, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat qy = glm::angleAxis(euler.y, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat qz = glm::angleAxis(euler.z, glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::quat q =
        qz * qy * qx; // match TransformComponent's ZYX composition

    const glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    const glm::mat4 R = glm::mat4_cast(q);
    const glm::mat4 worldNoScale = T * R;

    viewMatrix = glm::inverse(worldNoScale);
  } else {
    // Fallback: default camera at origin looking towards +Z with Y up
    // Note: keep consistent with right-handed convention used elsewhere
    const glm::vec3 position(0.0f);
    const glm::vec3 forward(0.0f, 0.0f, 1.0f);
    const glm::vec3 upVec(0.0f, 1.0f, 0.0f);
    viewMatrix = glm::lookAt(position, position + forward, upVec);
  }
  viewMatrixDirty = false;
}

void CameraComponent::UpdateProjectionMatrix() {
  projectionMatrix = glm::perspective(glm::radians(fieldOfView), aspectRatio,
                                      nearPlane, farPlane);
  projectionMatrixDirty = false;
}

