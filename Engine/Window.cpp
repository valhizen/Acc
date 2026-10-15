#include "Window.hpp"
#include "Core/InputEvent.hpp"
#include "Core/WindowEvent.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>

Window::Window() {}

bool Window::Initialize() {
  if (!glfwInit()) {
    throw std::runtime_error("Error Initializing GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(800, 800, "MyEngine", nullptr, nullptr);

  glfwSetWindowCloseCallback(window, [](GLFWwindow *handle) {
    Window &window = *((Window *)glfwGetWindowUserPointer(handle));

    WindowClosedEvent event;
    window.RaiseEvent(event);
  });

  glfwSetWindowSizeCallback(
      window, [](GLFWwindow *handle, int width, int height) {
        Window &window = *((Window *)glfwGetWindowUserPointer(handle));

        WindowResizeEvent event((uint32_t)width, (uint32_t)height);
        window.RaiseEvent(event);
      });

  glfwSetKeyCallback(window, [](GLFWwindow *handle, int key, int scancode,
                                int action, int mods) {
    Window &window = *((Window *)glfwGetWindowUserPointer(handle));

    switch (action) {
    case GLFW_PRESS:
    case GLFW_REPEAT: {
      KeyPressedEvent event(key, action == GLFW_REPEAT);
      window.RaiseEvent(event);
      break;
    }
    case GLFW_RELEASE: {
      KeyReleasedEvent event(key);
      window.RaiseEvent(event);
      break;
    }
    }
  });

  glfwSetMouseButtonCallback(
      window, [](GLFWwindow *handle, int button, int action, int mods) {
        Window &window = *((Window *)glfwGetWindowUserPointer(handle));

        switch (action) {
        case GLFW_PRESS: {
          MouseButtonPressedEvent event(button);
          window.RaiseEvent(event);
          break;
        }
        case GLFW_RELEASE: {
          MouseButtonReleasedEvent event(button);
          window.RaiseEvent(event);
          break;
        }
        }
      });

  glfwSetScrollCallback(
      window, [](GLFWwindow *handle, double xOffset, double yOffset) {
        Window &window = *((Window *)glfwGetWindowUserPointer(handle));

        MouseScrolledEvent event(xOffset, yOffset);
        window.RaiseEvent(event);
      });

  glfwSetCursorPosCallback(window, [](GLFWwindow *handle, double x, double y) {
    Window &window = *((Window *)glfwGetWindowUserPointer(handle));

    MouseMovedEvent event(x, y);
    window.RaiseEvent(event);
  });

  return true;
}

bool Window::ShouldClose() { return glfwWindowShouldClose(window); }

bool Window::Cleanup() {
  glfwDestroyWindow(window);
  glfwTerminate();
  return true;
}

void Window::RaiseEvent(Event &event) {
  if (m_Specification.EventCallback)
    m_Specification.EventCallback(event);
}
