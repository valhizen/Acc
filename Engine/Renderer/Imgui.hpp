#include <imgui/imgui.h>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

class Renderer;
struct ImGuiContext;
struct ImTextureData;

class ImGuiSystem {
public:
  ImGuiSystem();
  // Constructor-based initialization to replace separate Initialize() calls
  ImGuiSystem(Renderer *renderer, uint32_t width, uint32_t height) {
    if (!Initialize(renderer, width, height)) {
      throw std::runtime_error("ImGuiSystem: initialization failed");
    }
  }

  /**
   * @brief Destructor for proper cleanup.
   */
  ~ImGuiSystem();

  /**
   * @brief Clean up ImGui resources.
   */
  void Cleanup();

  /**
   * @brief Start a new ImGui frame.
   */
  void NewFrame();

  /**
   * @brief Render the ImGui frame.
   * @param commandBuffer The command buffer to record rendering commands to.
   */
  void Render(vk::raii::CommandBuffer &commandBuffer, uint32_t frameIndex);

  /**
   * @brief Handle mouse input.
   * @param x The x-coordinate of the mouse.
   * @param y The y-coordinate of the mouse.
   * @param buttons The state of the mouse buttons.
   */
  void HandleMouse(float x, float y, uint32_t buttons);

  /**
   * @brief Handle keyboard input.
   * @param key The key code.
   * @param pressed Whether the key was pressed or released.
   */
  void HandleKeyboard(uint32_t key, bool pressed);

  /**
   * @brief Handle character input.
   * @param c The character.
   */
  void HandleChar(uint32_t c);

  /**
   * @brief Handle window resize.
   * @param width The new width of the window.
   * @param height The new height of the window.
   */
  void HandleResize(uint32_t width, uint32_t height);

  /**
   * @brief Check if ImGui wants to capture keyboard input.
   * @return True if ImGui wants to capture keyboard input, false otherwise.
   */
  bool WantCaptureKeyboard() const;

  /**
   * @brief Check if ImGui wants to capture mouse input.
   * @return True if ImGui wants to capture mouse input, false otherwise.
   */
  bool WantCaptureMouse() const;

  /**
   * @brief Check if ImGui has already been rendered for the current frame.
   * @return True if Render() was already called in NewFrame(), false otherwise.
   */
  bool IsFrameRendered() const { return frameAlreadyRendered; }

private:
  /**
   * @brief Initialize the ImGui system (called by constructor).
   * @param renderer Pointer to the renderer.
   * @param width The width of the window.
   * @param height The height of the window.
   * @return True if initialization was successful, false otherwise.
   */
  bool Initialize(Renderer *renderer, uint32_t width, uint32_t height);

  // ImGui context
  ImGuiContext *context = nullptr;

  // Renderer reference
  Renderer *renderer = nullptr;

  // Vulkan resources
  vk::raii::DescriptorPool descriptorPool = nullptr;
  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::DescriptorSet descriptorSet = nullptr;
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  vk::raii::Pipeline pipeline = nullptr;
  vk::raii::Sampler fontSampler = nullptr;
  vk::raii::Image fontImage = nullptr;
  vk::raii::DeviceMemory fontMemory = nullptr;
  vk::raii::ImageView fontView = nullptr;
  // Per-frame dynamic buffers to avoid GPU/CPU contention when frames are in
  // flight
  std::vector<vk::raii::Buffer> vertexBuffers;
  std::vector<vk::raii::DeviceMemory> vertexBufferMemories;
  std::vector<vk::raii::Buffer> indexBuffers;
  std::vector<vk::raii::DeviceMemory> indexBufferMemories;
  std::vector<uint32_t> vertexCounts;
  std::vector<uint32_t> indexCounts;

  // Window dimensions
  uint32_t width = 0;
  uint32_t height = 0;

  // Mouse state
  float mouseX = 0.0f;
  float mouseY = 0.0f;
  uint32_t mouseButtons = 0;

  // Initialization flag
  bool initialized = false;

  // Track if ImGui::Render() was already called in NewFrame() (during loading
  // overlay)
  bool frameAlreadyRendered = false;
  /**
   * @brief Create Vulkan resources for ImGui.
   * @return True if creation was successful, false otherwise.
   */
  bool createResources();

  /**
   * @brief Create font texture.
   * @return True if creation was successful, false otherwise.
   */
  bool createFontTexture();

  /**
   * @brief Create descriptor set layout.
   * @return True if creation was successful, false otherwise.
   */
  bool createDescriptorSetLayout();

  /**
   * @brief Create descriptor pool.
   * @return True if creation was successful, false otherwise.
   */
  bool createDescriptorPool();

  /**
   * @brief Create descriptor set.
   * @return True if creation was successful, false otherwise.
   */
  bool createDescriptorSet();

  /**
   * @brief Create pipeline layout.
   * @return True if creation was successful, false otherwise.
   */
  bool createPipelineLayout();

  /**
   * @brief Create pipeline.
   * @return True if creation was successful, false otherwise.
   */
  bool createPipeline();

  /**
   * @brief Update vertex and index buffers.
   */
  void updateBuffers(uint32_t frameIndex);

  /**
   * @brief Handle dynamic texture updates from ImGui (v1.92+
   * RendererHasTextures protocol).
   * @param tex The texture data to update.
   */
  void UpdateTexture(ImTextureData *tex);
};
