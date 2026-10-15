#include "Engine.hpp"



Engine::Engine() {
  resourceManager = std::make_unique<ResourceManager>();
}

Engine::~Engine() {}

bool Engine::Initialize(const std::string &appName, int width, int height,
                             bool enableValidationLayers) {
  window = std::make_unique<Window>();
  window->Initialize();
  renderer = std::make_unique<SimpleEngineVK::Renderer>(window->getWindow());
  renderer->Initialize("engine", true);
  return true;
}

// void Engine::Run() {
//   running = true;

//   while (running) {
//     glfwPollEvents();
//     if (window->ShouldClose()) {
//       running = false;
//     }
//     renderer->Render();
//   }
// }
void Engine::Cleanup() {
  renderer.reset();
  window->Cleanup();
}

const ResourceManager *Engine::GetResourceManager() const {
  return resourceManager.get();
}

Entity *Engine::CreateEntity(const std::string &name) {
  // Always allow duplicate names; map stores a representative entity
  // Create the entity
  auto entity = std::make_unique<Entity>(name);
  // Add to the vector and map
  entities.push_back(std::move(entity));
  Entity *rawPtr = entities.back().get();
  // Update the map to point to the most recently created entity with this name
  entityMap[name] = rawPtr;
  return rawPtr;
}



