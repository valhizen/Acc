#include "Engine/Engine.hpp"

#include "../Engine/Logger.hpp"
#include <../Engine/Entity/Components/Transform.hpp>
#include <../Engine/Entity/Components/Camera.hpp>
#include <../Engine/Entity/ECS.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

void SetupScene(Engine *engine) {
  Entity *cameraEntity = engine->CreateEntity("Camera");
  if (!cameraEntity) {
    throw std::runtime_error("Failed to create camera entity");

    // Add a transform component to the camera
    auto *cameraTransform =
        cameraEntity->AddComponent<TransformComponent>();
    cameraTransform->SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));
  }

	// Add a camera component to the camera entity
  auto *camera = cameraEntity->AddComponent<CameraComponent>();
    // Camera aspect ratio will be set by the engine during initialization or resize events.
    // Set the camera as the active camera
}

int main() {
  logInfo("Main Function Entered");

  Engine app;
  SetupScene(&app);  
  if (app.Initialize("SimpleEngine", 800, 800, true)) {
    // app.Run();
  }
  app.Cleanup();
}
