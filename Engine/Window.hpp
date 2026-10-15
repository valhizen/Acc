#pragma once

#include "Core/Event.hpp"
#include <GLFW/glfw3.h>

struct WindowSpecification {

  std::string Title;
  uint32_t Width = 1280;
  uint32_t Height = 720;
  bool IsResizeable = true;
  bool VSync = true;

  using EventCallbackFn = std::function<void(Event &)>;
  EventCallbackFn EventCallback;
};

class Window {
public:
  Window();
  GLFWwindow *getWindow() const { return window; }
  bool ShouldClose();

  bool Initialize();
  bool Cleanup();

  void RaiseEvent(Event &event);

private:
  GLFWwindow *window;
  WindowSpecification m_Specification;  
};
