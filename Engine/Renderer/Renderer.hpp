#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

#include "VMAImpl.hpp"

#include <glm/glm.hpp>

#include <atomic>
#include <optional>
#include <vector>

/**
 * @brief Structure for Vulkan queue family indices.
 */


struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  std::optional<uint32_t> computeFamily;
  std::optional<uint32_t>
      transferFamily; // optional dedicated transfer queue family

  [[nodiscard]] bool isComplete() const {
    return graphicsFamily.has_value() && presentFamily.has_value() &&
           computeFamily.has_value();
  }
};

/**
 * @brief Structure for swap chain support details.
 */
struct SwapChainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
};

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 projection;
  alignas(16) glm::mat4 cameraPosition;
};

class Renderer {
public:
  Renderer(GLFWwindow *window);
  ~Renderer();
  bool Initialize(const std::string &appName, bool enableValidationLayer);
  void Cleanup();

  void Render();

  void WaitIdle();
  /**
   * @brief Wait for fences with periodic watchdog kicks to prevent false hang
   * detection. Must be called from the render thread.
   */
  vk::Result WaitForFencesSafe(const std::vector<vk::Fence> &fences,
                               vk::Bool32 waitAll,
                               uint64_t timeoutNs = 100'000'000ULL);

  /**
   * @brief Wait for fences with periodic watchdog kicks to prevent false hang
   * detection. Must be called from the render thread. Overload for a single
   * fence.
   */
  vk::Result WaitForFencesSafe(vk::Fence fence, vk::Bool32 waitAll,
                               uint64_t timeoutNs = 100'000'000ULL);

  bool IsInitialized() const { return initialized; }
  /**
   * @brief Get the Vulkan device.
   * @return The Vulkan device.
   */
  vk::Device GetDevice() const { return *device; }

  // Expose max frames in flight for per-frame resource duplication
  uint32_t GetMaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }

  /**
   * @brief Get the Vulkan RAII device.
   * @return The Vulkan RAII device.
   */
  const vk::raii::Device &GetRaiiDevice() const { return device; }

  // Expose uploads timeline semaphore and last value for external waits
  vk::Semaphore GetUploadsTimelineSemaphore() const { return *uploadsTimeline; }
  uint64_t GetUploadsTimelineValue() const {
    return uploadTimelineLastSubmitted.load(std::memory_order_relaxed);
  }

  /**
   * @brief Get the current command buffer.
   * @return The current command buffer.
   */
  vk::raii::CommandBuffer &GetCurrentCommandBuffer() {
    return commandBuffers[currentFrame];
  }

  /**
   * @brief Get the swap chain image format.
   * @return The swap chain image format.
   */
  vk::Format GetSwapChainImageFormat() const { return swapChainImageFormat; }

  /**
   * @brief Set the framebuffer resized flag.
   * This should be called when the window is resized to trigger swap chain
   * recreation.
   */

private:
  GLFWwindow *window{nullptr};
  vk::raii::Context context;
  vk::raii::Instance instance{nullptr};
  vk::raii::SurfaceKHR surface{nullptr};
  vk::raii::PhysicalDevice physicalDevice{nullptr};
  vk::raii::Device device{nullptr};

  // Swap chain
  vk::raii::SwapchainKHR swapChain = nullptr;
  std::vector<vk::Image> swapChainImages;
  vk::Format swapChainImageFormat = vk::Format::eUndefined;
  vk::Extent2D swapChainExtent = {0, 0};
  std::vector<vk::raii::ImageView> swapChainImageViews;
  // Tracked layouts for swapchain images (VVL requires correct oldLayout in
  // barriers). Initialized at swapchain creation and updated as we transition.
  std::vector<vk::ImageLayout> swapChainImageLayouts;

  QueueFamilyIndices queueFamilyIndices;

  // Maximum number of frames in flight
  // More than 1 allows CPU/GPU overlap and reduce per-frame stalls.
  // All per-frame resources (UBOs, descriptor sets, reflection RTs, etc.)
  // are sized dynamically based on this value.
  const uint32_t MAX_FRAMES_IN_FLIGHT = 2u;

  // Vulkan queues
  vk::raii::Queue graphicsQueue{nullptr};
  vk::raii::Queue presentQueue{nullptr};
  vk::raii::Queue computeQueue{nullptr};
  // Dedicated transfer queue (falls back to graphics if unavailable)
  vk::raii::Queue transferQueue{nullptr};

  // Upload timeline semaphore for transfer -> graphics handoff (signaled per
  // upload)
  vk::raii::Semaphore uploadsTimeline{nullptr};
  // Tracks last timeline value that has been submitted for signaling on
  // uploadsTimeline
  std::atomic<uint64_t> uploadTimelineLastSubmitted{0};

  // Command pool and buffers
  vk::raii::CommandPool commandPool = nullptr;
  std::vector<vk::raii::CommandBuffer> commandBuffers;

  // Synchronization objects
  std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
  std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
  std::vector<vk::raii::Fence> inFlightFences;

  std::unique_ptr<MemoryPool> memoryPool;

  // Private Methods

  bool CreateInstance(const std::string &appName, bool enableValidationLayer);
  bool SetupDebugMessenger(bool enableValidationLayer);
  bool CheckValidationLayerSupport();
  bool CreateSurface();
  bool PickPhysicalDevice();
  void AddSupportedOptionalExtensions();
  bool CreateLogicalDevice(bool enableValidationLayers);
  bool CreateSwapchain();
  bool CreateImageViews();
  bool SetupDynamicRendering();
  bool CreateDescriptorSetLayout();
  bool CreateGraphicsPipeline();
  bool CreateDepthResources();
  void ReCreateSwapchain();

  bool CreateCommandBuffers();
  bool CreateCommandPool();
  bool CreateSyncObjects();

  void CleanupSwapChain();

  QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice &device);
  bool checkDeviceExtensionSupport(vk::raii::PhysicalDevice &device);
  SwapChainSupportDetails
  querySwapChainSupport(const vk::raii::PhysicalDevice &device);

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR> &availableFormats);
  vk::PresentModeKHR chooseSwapPresentMode(
      const std::vector<vk::PresentModeKHR> &availablePresentModes);
  vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);

  std::vector<char> readFile(const std::string &filename);
  vk::raii::ShaderModule CreateShaderModule(const std::vector<char> &code);

  vk::Format FindSupportedFormat(const std::vector<vk::Format> &candidates,
                                 vk::ImageTiling tiling,
                                 vk::FormatFeatureFlags features);
  vk::Format FindDepthFormat();
  bool HasStencilComponent(vk::Format format);
  uint32_t FindMemoryType(uint32_t typeFilter,
                          vk::MemoryPropertyFlags properties);

  bool CreateDescriptorPool();
  bool CreateDescriptorSets();

  bool CreateUniformBuffers();
  void UpdateUniformBuffers();

  // std::pair<vk::raii::Image, std::unique_ptr<MemoryPool::Allocation>>
  const std::vector<char const *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

  // Required device extensions
  const std::vector<const char *> requiredDeviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};

  // All device extensions (required + optional)
  std::vector<const char *> deviceExtensions;

  // Pipelines
  vk::raii::PipelineLayout pipelineLayout = nullptr;
  vk::raii::Pipeline graphicsPipeline = nullptr;

  bool initialized = false;

  // Descriptor set layouts (declared before pools and sets)
  vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
  vk::raii::DescriptorSetLayout pbrDescriptorSetLayout = nullptr;
  vk::raii::DescriptorSetLayout transparentDescriptorSetLayout = nullptr;
  vk::raii::PipelineLayout pbrTransparentPipelineLayout = nullptr;

  // Whether VK_EXT_descriptor_indexing (update-after-bind) path is enabled
  bool descriptorIndexingEnabled = false;
  bool descriptorBindingUniformBufferUpdateAfterBindEnabled = false;
  bool descriptorBindingSampledImageUpdateAfterBindEnabled = false;
  bool storageAfterBindEnabled = false;
  // Feature toggles detected/enabled at device creation
  bool robustness2Enabled = false;
  bool dynamicRenderingLocalReadEnabled = false;
  bool shaderTileImageEnabled = false;
  bool rayQueryEnabled = false;
  bool accelerationStructureEnabled = false;

  std::vector<vk::raii::Buffer> uniformBuffers;
  std::vector<VmaAllocation> uniformBuffersMemory;
  std::vector<void *> uniformBuffersMapped;

  // std::vector<vk::raii::DeviceMemory> opaqueSceneColorImageAllocations;
  // std::vector<vk::raii::Image> opaqueSceneColorImages;
  // std::vector<vk::raii::ImageView> opaqueSceneColorImageViews;
  //
  // Track the current layout per frame (initialized to eUndefined at creation)
  std::vector<vk::ImageLayout> opaqueSceneColorImageLayouts;
  vk::raii::Sampler opaqueSceneColorSampler{nullptr};

  // Dynamic rendering info
  vk::RenderingInfo renderingInfo;
  std::vector<vk::RenderingAttachmentInfo> colorAttachments;
  vk::RenderingAttachmentInfo depthAttachment;

  // Descriptor pool (declared after entity resources to ensure proper
  // destruction order)
  vk::raii::DescriptorPool descriptorPool = nullptr;

  uint32_t currentFrame = 0;

  // Depth buffer
  vk::Image depthImage = nullptr;
  vk::raii::ImageView depthImageView = nullptr;
  VmaAllocation depthAllocation{nullptr};
  vk::Format depthImageFormat = vk::Format::eUndefined;

  // Optional device extensions
  std::vector<const char *> optionalDeviceExtensions = {
      VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
      VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
      VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
      // Robustness and safety
      VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
      // Tile/local memory friendly dynamic rendering readback
      VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME,
      // Shader tile image for fast tile access
      VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME,
      // Ray query support for ray-traced rendering
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
      VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
      VK_KHR_RAY_QUERY_EXTENSION_NAME};
};
