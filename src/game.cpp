#include "Engine/Engine.hpp"

// Game code
int main() {

  Engine engine;
  engine.Initialize("Engine", 800, 800, true);

  // Entity *player = engine.CreateEntity("Player");
  // if (player) {
  //   player->AddComponent<PlayerTag>("Sarbesh");
  //   player->AddComponent<TransformComponent>();
  // }
  // SceneManager sceneManager;

  bool running = true;

  while (running) {
    // engine.render(sceneManager.scene); // 
  }  
}


