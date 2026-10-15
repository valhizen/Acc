#include "ECS.hpp"



void Entity::Initialize() {
  for (auto &component : components) {
    component->Initialize();
  }
}

void Entity::Update(std::chrono::milliseconds deltaTime) {
  if (!active) {

    return;
  }
  for (auto &component : components) {
    if (component->IsActive()) {
      component->Update(deltaTime);
    }
  }
}

void Entity::Render() {
  if (!active) {

    return;
  }

  for (auto &component : components) {
    if (component->IsActive()) {
      component->Render();
    }
  }
}

