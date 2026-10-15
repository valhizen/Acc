#pragma once


#include "Event.hpp"

class Scene {
public:
  virtual void Render();
  virtual void Update(float deltaTime);
  virtual void HandelEvent(Event &event);  
  virtual void Destroy();  
};
