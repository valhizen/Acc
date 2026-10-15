#include "Imgui.hpp"
#include "Renderer.hpp"
#include <iostream>

ImGuiSystem::ImGuiSystem() {
  // Constructor implementation
}

ImGuiSystem::~ImGuiSystem() {
  // Destructor implementation
  Cleanup();
}

bool ImGuiSystem::Initialize(Renderer *renderer, uint32_t width,
                             uint32_t height) {
  if (initialized) {
    return true;
  }

  this->renderer = renderer;
  this->width = width;
  this->height = height;

  // Create ImGui context
  context = ImGui::CreateContext();
  if (!context) {
    std::cerr << "Failed to create ImGui context" << std::endl;
    return false;
  }

  // Configure ImGui
  ImGuiIO &io = ImGui::GetIO();
  // Set display size
  io.DisplaySize =
      ImVec2(static_cast<float>(width), static_cast<float>(height));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

  // Inform ImGui that we support the new texture update protocol (v1.92+)
  io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

  // Set up ImGui style
  ImGui::StyleColorsDark();

#if defined(PLATFORM_ANDROID)
  // Scale UI for high-DPI mobile screens to ensure buttons are touchable
  ImGui::GetStyle().ScaleAllSizes(2.0f);
  io.FontGlobalScale = 2.0f;
#endif

  // Create Vulkan resources
  if (!createResources()) {
    std::cerr << "Failed to create ImGui Vulkan resources" << std::endl;
    Cleanup();
    return false;
  }

  // Initialize per-frame buffers containers
  if (renderer) {
    uint32_t frames = renderer->GetMaxFramesInFlight();
    vertexBuffers.clear();
    vertexBuffers.reserve(frames);
    vertexBufferMemories.clear();
    vertexBufferMemories.reserve(frames);
    indexBuffers.clear();
    indexBuffers.reserve(frames);
    indexBufferMemories.clear();
    indexBufferMemories.reserve(frames);
    for (uint32_t i = 0; i < frames; ++i) {
      vertexBuffers.emplace_back(nullptr);
      vertexBufferMemories.emplace_back(nullptr);
      indexBuffers.emplace_back(nullptr);
      indexBufferMemories.emplace_back(nullptr);
    }
    vertexCounts.assign(frames, 0);
    indexCounts.assign(frames, 0);
  }

  initialized = true;
  return true;
}

void ImGuiSystem::Cleanup() {
  if (!initialized) {
    return;
  }

  // Wait for the device to be idle before cleaning up
  if (renderer) {
    renderer->WaitIdle();
  }
  // Destroy ImGui context
  if (context) {
    ImGui::DestroyContext(context);
    context = nullptr;
  }

  initialized = false;
}

void ImGuiSystem::NewFrame() {
  if (!initialized) {
    return;
  }

  frameAlreadyRendered = false;

  ImGui::NewFrame();

  if (renderer) {
    ImGuiIO &io = ImGui::GetIO();

    const ImVec2 dispSize = io.DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(dispSize);

    // Override style for loading overlay to ensure visibility and contrast
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;
  }

  ImGui::Begin("Just a Test Window");
  ImGui::End();
}

void ImGuiSystem::Render(vk::raii::CommandBuffer &commandBuffer,
                         uint32_t frameIndex) {
  if (!initialized) {
    return;
  }
  // End the frame and prepare for rendering
  ImGui::Render();
  // Update vertex and index buffers for this frame
  updateBuffers(frameIndex);
}
