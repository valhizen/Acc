#pragma once

#include "Engine/Core/Scene.hpp"


// Scene Main Menu;


class MainMenu : public Scene {

public:
  void Render();
  void Update(float deltaTime);
  void HandelEvent();
  void Destroy();

private:
  // Refrence to all the things this scene needs
  std::string backgroundImgPath;
  std::string fontPath;
};

