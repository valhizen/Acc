#pragma once

#include "Component.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>


class Entity {
private:
  std::string name;
  bool active = true;
  std::vector<std::unique_ptr<Component>> components;

public:
  explicit Entity(const std::string &entityName) : name{entityName} {}

  virtual ~Entity() = default;

  const std::string &GetName() { return name; }

  bool IsActive() { return active; }

  void SetActive(bool isActive) { active = isActive; }

  void Initialize();

  void Update(std::chrono::milliseconds deltaTime);

  void Render();

  template <typename T, typename... Args> T *AddComponent(Args &&...args) {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must derive from component");

    // create the component
    auto component = std::make_unique<T>(std::forward<Args>(args)...);
    T *componentPtr = component.get();
    // set the owner
    componentPtr->SetOwner(this);
    // add to vector for ownership and iteration
    components.push_back(std::move(component));
    // initialize the component
    componentPtr->Initialize();

    return componentPtr;
  }

  template <typename T> T *GetComponent() const {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must derive from component");

    for (auto it = components.rbegin(); it != components.rend(); ++it) {
      if (auto *casted = dynamic_cast<T *>(it->get())) {

        return casted;
      }
    }
    return nullptr;
  }

  template <typename T> bool RemoveComponent() {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must derive from component");

    for (auto it = components.rbegin(); it != components.rend(); ++it) {
      if (dynamic_cast<T *>(it->get()) != nullptr) {
        components.erase(std::next(it).base());
        return true;
      }
    }
    return false;
  }

  template <typename T> bool HasComponent() const {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must derive from component");
    return GetComponent<T>() != nullptr;
  }
};

