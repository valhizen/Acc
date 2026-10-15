#pragma once

#include "Core/ResourceManager.hpp"
#include "Entity/ECS.hpp"
#include "Renderer/Renderer.hpp"
#include "Window.hpp"

class Engine {
public:
  using TimeDelta = std::chrono::milliseconds;  
  Engine();
  ~Engine();
  bool Initialize(const std::string &appName, int width, int height,
                  bool enableValidationLayers = true);
  void Cleanup();

  Entity *CreateEntity(const std::string &name);
private:
  // SubSystems
  std::unique_ptr<Renderer> renderer;
  std::unique_ptr<Window> window;
  std::unique_ptr<ResourceManager> resourceManager;
  std::vector<std::unique_ptr<Entity>> entities;
  std::unordered_map<std::string, Entity *> entityMap;

  bool running{false};

  const ResourceManager *GetResourceManager() const;
};
