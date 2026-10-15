#pragma once

#include <chrono>
#include <string>


class Entity;

class Component {

protected:
  Entity *owner = nullptr;
  std::string name;
  bool active = true;

public:
  explicit Component(const std::string &componentName = "Component")
      : name{componentName} {}

  virtual ~Component() = default;

  virtual void Initialize() {}

  virtual void Update(std::chrono::milliseconds deltaTime) {}

  virtual void Render() {}

  void SetOwner(Entity *entity) { owner = entity; }

  Entity *GetOwner() const { return owner; }

  const std::string &GetName() { return name; }

  bool IsActive() const { return active; }
  void SetActive(bool IsActive) { active = IsActive; }
};

