#include "Transform.hpp"

const glm::mat4 &TransformComponent::GetModelMatrix() {
  if (matrixDirty) {
    UpdateModelMatrix();
  }
  return modelMatrix;
}

void TransformComponent::UpdateModelMatrix() {
  // Compose rotation with quaternions for stability and to avoid rad/deg
  // ambiguity
  glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
  glm::quat qx = glm::angleAxis(rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::quat qy = glm::angleAxis(rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::quat qz = glm::angleAxis(rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
  glm::quat q = qz * qy * qx; // ZYX order is conventional for Euler composition
  glm::mat4 R = glm::mat4_cast(q);
  glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
  modelMatrix = T * R * S;
  matrixDirty = false;
}

