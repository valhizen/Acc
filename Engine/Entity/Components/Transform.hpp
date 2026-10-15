#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


#include "../Component.hpp"



class TransformComponent : public Component {
private:
  glm::vec3 position = {0.0f, 0.0f, 0.0f};
  glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
  glm::vec3 scale = {1.0f, 1.0f, 1.0f};

  glm::mat4 modelMatrix = glm::mat4(1.0f);
  bool matrixDirty = true;

public:
  explicit TransformComponent(
      const std::string &componentName = "TransformComponent")
      : Component(componentName) {}

  void SetPosition(const glm::vec3 &newPosition) {
    position = newPosition;
    matrixDirty = true;
  }

  const glm::vec3 &GetPosition() const { return position; }

  void SetRotation(const glm::vec3 &newRotation) {
    rotation = newRotation;
    matrixDirty = true;
  }

  const glm::vec3 &GetRotation() const { return rotation; }
  void SetScale(const glm::vec3 &newScale) {
    scale = newScale;
    matrixDirty = true;
  }
  const glm::vec3 &GetScale() const { return scale; }
  void SetUniformScale(float uniformScale) {
    scale = glm::vec3(uniformScale);
    matrixDirty = true;
  }
  void Translate(const glm::vec3 &translation) {
    position += translation;
    matrixDirty = true;
  }
  void Rotate(const glm::vec3 &eulerAngles) {
    rotation += eulerAngles;
    matrixDirty = true;
  }
  void Scale(const glm::vec3 &scaleFactors) {
    scale *= scaleFactors;
    matrixDirty = true;
  }
  const glm::mat4 &GetModelMatrix();

private:
  void UpdateModelMatrix();
};

