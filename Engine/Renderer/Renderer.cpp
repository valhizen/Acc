#include "Renderer.hpp"
#include "../Logger.hpp"

#include <map>
#include <set>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

namespace SimpleEngineVK {

// Debug callback for vk::raii - uses raw Vulkan C types for cross-platform
// compatibility
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackVkRaii(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    [[maybe_unused]] void *pUserData) {
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    // Print a message to the console
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
  } else {
    // Print a message to the console
    std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;
  }

  return VK_FALSE;
}

// Vulkan-Hpp style callback signature for newer headers expecting vk:: types
static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallbackVkHpp(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    [[maybe_unused]] void *pUserData) {
  if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
  } else {
    std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;
  }
  return vk::False;
}

Renderer::Renderer(GLFWwindow *window) : window(window) {
  deviceExtensions = requiredDeviceExtensions;
}

Renderer::~Renderer() { Cleanup(); }

bool Renderer::Initialize(const std::string &appName,
                          bool enableVaidationLayers) {
  logInfo("Renderer:: Initialization Start");

  VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

  logInfo("Creating Vulkan Instance ... ");
  if (!CreateInstance(appName, enableVaidationLayers)) {
    logError("Failed to Create Vulkan Instance");
    return false;
  }

  logInfo("Creating Debug Messenger ... ");
  if (!SetupDebugMessenger(enableVaidationLayers)) {
    logError("Failed to Create Vulkan Debug Messenger");
    return false;
  }

  logInfo("Creating Vulkan Surface ... ");
  if (!CreateSurface()) {
    logError("Failed to Create Vulkan Surface");
    return false;
  }

  logInfo("Picking Physical Device ... ");
  if (!PickPhysicalDevice()) {
    logError("Failed to Pick Physical Device");
    return false;
  }

  logInfo("Creating Logical Device ... ");
  if (!CreateLogicalDevice(enableVaidationLayers)) {
    logError("Failed to Create Logical Device");
    return false;
  }

  logInfo("Initializing Allocator");
  memoryPool = std::make_unique<MemoryPool>(device, physicalDevice, instance);
  if (!memoryPool->CreateVmaAllocator()) {
    logError("Failed to create allocator");
    return false;
  }

  logInfo("Creating SwapChain ... ");
  if (!CreateSwapchain()) {
    logError("Failed to Create Swapchain");
    return false;
  }

  logInfo("Creating Create Image ... ");
  if (!CreateImageViews()) {
    logError("Failed to Create Image Views");
    return false;
  }

  logInfo("Creating Depth Resources ... ");
  if (!CreateDepthResources()) {
    logError("Failed to Create Depth Resources");
    return false;
  }
  logInfo("Creating Descriptor Pool ... ");
  if (!CreateDescriptorPool()) {
    logError("Failed to Create Descriptor Pool;");
    return false;
  }

  // logInfo("Creating Opaque Scene Color Resources ... ");
  // if (!CreateOpaqueSceneColorResources()) {
  //   logError("Failed to Create Opaque Scene Color Resources");
  //   return false;
  // }

  logInfo("Setting up Dynamic Rendering ... ");
  if (!SetupDynamicRendering()) {
    logError("Failed to Setup Dynamic Rendering");
    return false;
  }

  logInfo("Creating Descriptor Set Layout... ");
  if (!CreateDescriptorSetLayout()) {
    logError("Failed to create descriptor set layout");
    return false;
  }

  logInfo("Creating Graphics Pipeline ... ");
  if (!CreateGraphicsPipeline()) {
    logError("Failed to create graphics pipeline");
    return false;
  }

  logInfo("Creating Command Pool... ");
  if (!CreateCommandPool()) {
    logError("Failed to create command pool");
    return false;
  }

  logInfo("Creating Command Buffers ... ");
  if (!CreateCommandBuffers()) {
    logError("Failed to create command buffers");
    return false;
  }

  logInfo("Creating Sync Object... ");
  if (!CreateSyncObjects()) {
    logError("Failed to create Sync Object");
    return false;
  }
  initialized = true;

  return true;
}

bool Renderer::CreateInstance(const std::string &appName,
                              bool enableValidationLayers) {
  // Create Application Info
  vk::ApplicationInfo appInfo{.pApplicationName = appName.c_str(),
                              .applicationVersion =
                                  VK_MAKE_API_VERSION(0, 1, 4, 0),
                              .pEngineName = "MyEngine",
                              .engineVersion = VK_MAKE_API_VERSION(0, 1, 4, 0),
                              .apiVersion = vk::ApiVersion14};

  // Get Required extensions
  std::vector<const char *> extensions;

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  extensions.insert(extensions.end(), glfwExtensions,
                    glfwExtensions + glfwExtensionCount);

  // Add Debug Extensions if validation layer is enabled
  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  vk::InstanceCreateInfo createInfo = {
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data()};

  // Enable validation layer is requested
  vk::ValidationFeaturesEXT validationFeatures{};
  std::vector<vk::ValidationFeatureEnableEXT> enabledValidationFeatures;

  bool actualEnableValidatoionLayer = enableValidationLayers;

  if (actualEnableValidatoionLayer) {
    if (!CheckValidationLayerSupport()) {
      logWarning(
          "Validation Layer requested but not avilable, continue without "
          "validation");
      actualEnableValidatoionLayer = false;
    }
  }

  if (actualEnableValidatoionLayer) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    validationFeatures.enabledValidationFeatureCount =
        static_cast<uint32_t>(enabledValidationFeatures.size());
    validationFeatures.pEnabledValidationFeatures =
        enabledValidationFeatures.data();

    createInfo.pNext = &validationFeatures;
  }

  instance = vk::raii::Instance(context, createInfo);

  VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

  return true;
}

bool Renderer::SetupDebugMessenger(bool enableValidationLayers) {
  if (!enableValidationLayers) {
    return true;
  }

  // Create debug messenger info
  vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.messageSeverity =
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
  createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

  // Select callback: modern Vulkan-Hpp expects vk:: types.
  createInfo.pfnUserCallback = &debugCallbackVkHpp;

  // Create debug messenger
  debugMessenger = vk::raii::DebugUtilsMessengerEXT(instance, createInfo);
  return true;
}

void Renderer::Cleanup() {
  if (!initialized) {
    return;
  }

  logInfo("Starting render cleanup ...");

  WaitIdle();

  CleanupSwapChain();

  commandBuffers.clear();
  commandPool = nullptr;

  imageAvailableSemaphores.clear();
  renderFinishedSemaphores.clear();
  inFlightFences.clear();
  uploadsTimeline = nullptr;

  graphicsQueue = nullptr;
  presentQueue = nullptr;
  computeQueue = nullptr;
  transferQueue = nullptr;
  surface = nullptr;

  initialized = false;
  std::cout << "Renderer cleanup completed." << "\n";
}

bool Renderer::CreateSurface() {
  // Create surface
  VkSurfaceKHR _surface;
  if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) !=
      VK_SUCCESS) {
    logError("Failed to Create Window Surface", glfwGetError(nullptr));
    return false;
  }

  surface = vk::raii::SurfaceKHR(instance, _surface);
  return true;
}

bool Renderer::PickPhysicalDevice() {
  std::vector<vk::raii::PhysicalDevice> devices =
      instance.enumeratePhysicalDevices();

  if (devices.empty()) {
    std::cerr << "Failed to find GPUs with Vulkan support" << std::endl;
    return false;
  }

  // Prioritize discrete GPUs (like NVIDIA RTX 2080) over integrated GPUs
  // (like Intel UHD Graphics) First, collect all suitable devices with their
  // suitability scores
  std::multimap<int, vk::raii::PhysicalDevice> suitableDevices;

  for (auto &_device : devices) {
    vk::PhysicalDeviceProperties deviceProperties = _device.getProperties();

    std::cout << "Checking Device : %s" << deviceProperties.deviceName.data();

    // Check if the device supports Vulkan 1.4
    bool supportsVulkan1_4 = deviceProperties.apiVersion >= VK_API_VERSION_1_4;
    if (!supportsVulkan1_4) {
      logError(" - Doesn't Support Vulkan 1.4");
      continue;
    }

    // Check Queue Families
    QueueFamilyIndices indices = findQueueFamilies(_device);
    bool supportsGraphics = indices.isComplete();

    if (!supportsGraphics) {
      logError("  - Missing required queue families");
      continue;
    }

    // Check device extensions
    bool supportsAllRequiredExtensions = checkDeviceExtensionSupport(_device);
    if (!supportsAllRequiredExtensions) {
      logError("  - Missing required extensions");
      continue;
    }

    // Check swap chain support
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_device);

    bool swapChainAdequate = !swapChainSupport.formats.empty() &&
                             !swapChainSupport.presentModes.empty();
    if (!swapChainAdequate) {
      logError("  - Inadequate swap chain support");
      continue;
    }

    // Check for required features
    auto features = _device.getFeatures2<vk::PhysicalDeviceFeatures2,
                                         vk::PhysicalDeviceVulkan13Features>();
    bool supportsRequiredFeatures =
        features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering;
    if (!supportsRequiredFeatures) {
      logError("  - Does not support required features (dynamicRendering)");
      continue;
    }

    // Calculate suitability score - prioritize discrete GPUs
    int score = 0;

    // Discrete GPUs get the highest priority (NVIDIA RTX 2080, AMD, etc.)
    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      score += 1000;
      logInfo("  - Discrete GPU: +1000 points");
    }
    // Integrated GPUs get lower priority (Intel UHD Graphics, etc.)
    else if (deviceProperties.deviceType ==
             vk::PhysicalDeviceType::eIntegratedGpu) {
      score += 100;
      logInfo("  - Integrated GPU: +100 points");
    }

    // Add points for memory size (more VRAM is better)
    vk::PhysicalDeviceMemoryProperties memProperties =
        _device.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) {
      if (memProperties.memoryHeaps[i].flags &
          vk::MemoryHeapFlagBits::eDeviceLocal) {
        // Add 1 point per GB of VRAM
        score += static_cast<int>(memProperties.memoryHeaps[i].size /
                                  (1024 * 1024 * 1024));
        break;
      }
    }

    logInfo("  - Device is suitable with score");
    std::cout << score << "\n";
    suitableDevices.emplace(score, _device);
  }

  if (!suitableDevices.empty()) {
    // Select the device with the highest score (discrete GPU with most VRAM)
    physicalDevice = suitableDevices.rbegin()->second;
    vk::PhysicalDeviceProperties deviceProperties =
        physicalDevice.getProperties();

    // Store queue family indices for the selected device
    queueFamilyIndices = findQueueFamilies(physicalDevice);

    // Add supported optional extensions
    AddSupportedOptionalExtensions();

    return true;
  }
  logError(
      "Failed to find a suitable GPU. Make sure your GPU supports Vulkan and "
      "has the required extensions.");

  return false;
}

// Add supported optional extensions
void Renderer::AddSupportedOptionalExtensions() {
  // Get available extensions
  auto availableExtensions =
      physicalDevice.enumerateDeviceExtensionProperties();

  // Build a set of available extension names for quick lookup
  std::set<std::string> avail;
  for (const auto &e : availableExtensions) {
    avail.insert(e.extensionName);
  }

  for (const auto &optionalExt : optionalDeviceExtensions) {
    if (avail.contains(optionalExt)) {
      deviceExtensions.push_back(optionalExt);
      std::cout << "Adding optional extension: " << optionalExt << std::endl;
    }
  }
}

bool Renderer::CreateLogicalDevice(bool enableValidationLayers) {
  logInfo("CreateLogicalDevice function entere");

  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(),
                                  queueFamilyIndices.presentFamily.value(),
                                  queueFamilyIndices.computeFamily.value(),
                                  queueFamilyIndices.transferFamily.value()

  };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {

    vk::DeviceQueueCreateInfo queueCreateInfo{.queueFamilyIndex = queueFamily,
                                              .queueCount = 1,
                                              .pQueuePriorities =
                                                  &queuePriority};
    queueCreateInfos.push_back(queueCreateInfo);
  }

  logInfo("Querying supported Features");

  // Query supported features before enabling them
  auto featureChainSupported = physicalDevice.getFeatures2<
      vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceTimelineSemaphoreFeatures,
      vk::PhysicalDeviceVulkanMemoryModelFeatures,
      vk::PhysicalDeviceBufferDeviceAddressFeatures,
      vk::PhysicalDevice8BitStorageFeatures, vk::PhysicalDeviceVulkan11Features,
      vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features,
      vk::PhysicalDeviceDescriptorIndexingFeatures,
      vk::PhysicalDeviceRobustness2FeaturesEXT,
      vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR,
      vk::PhysicalDeviceShaderTileImageFeaturesEXT,
      vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
      vk::PhysicalDeviceRayQueryFeaturesKHR>();

  logInfo("Featured queried successfully");

  // Extract supported feature structs
  const auto &coreSupported =
      featureChainSupported.get<vk::PhysicalDeviceFeatures2>().features;
  const auto &timelineSupported =
      featureChainSupported.get<vk::PhysicalDeviceTimelineSemaphoreFeatures>();
  const auto &memoryModelSupported =
      featureChainSupported.get<vk::PhysicalDeviceVulkanMemoryModelFeatures>();
  const auto &bufferAddressSupported =
      featureChainSupported
          .get<vk::PhysicalDeviceBufferDeviceAddressFeatures>();
  const auto &storage8BitSupported =
      featureChainSupported.get<vk::PhysicalDevice8BitStorageFeatures>();
  const auto &vulkan11Supported =
      featureChainSupported.get<vk::PhysicalDeviceVulkan11Features>();
  const auto &vulkan13Supported =
      featureChainSupported.get<vk::PhysicalDeviceVulkan13Features>();
  const auto &indexingFeaturesSupported =
      featureChainSupported.get<vk::PhysicalDeviceDescriptorIndexingFeatures>();
  const auto &robust2Supported =
      featureChainSupported.get<vk::PhysicalDeviceRobustness2FeaturesEXT>();
  const auto &localReadSupported =
      featureChainSupported
          .get<vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR>();
  const auto &tileImageSupported =
      featureChainSupported.get<vk::PhysicalDeviceShaderTileImageFeaturesEXT>();
  const auto &accelerationStructureSupported =
      featureChainSupported
          .get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
  const auto &rayQuerySupported =
      featureChainSupported.get<vk::PhysicalDeviceRayQueryFeaturesKHR>();

  // Verify critical features are supported
  if (!coreSupported.samplerAnisotropy)
    logWarning("Missing feature: samplerAnisotropy");
  if (!timelineSupported.timelineSemaphore)
    logWarning("Missing feature: timelineSemaphore");
  if (!memoryModelSupported.vulkanMemoryModel)
    logWarning("Missing feature: vulkanMemoryModel");
  if (!bufferAddressSupported.bufferDeviceAddress)
    logWarning("Missing feature: bufferDeviceAddress");
  if (!vulkan13Supported.dynamicRendering)
    logWarning("Missing feature: dynamicRendering");
  if (!vulkan13Supported.synchronization2)
    logWarning("Missing feature: synchronization2");

  if (!coreSupported.samplerAnisotropy ||
      !timelineSupported.timelineSemaphore ||
      !memoryModelSupported.vulkanMemoryModel ||
      !bufferAddressSupported.bufferDeviceAddress ||
      !vulkan13Supported.dynamicRendering ||
      !vulkan13Supported.synchronization2) {
  }

  // Helper to check extension availability
  auto hasExtension = [&](const char *name) {
    return std::find_if(deviceExtensions.begin(), deviceExtensions.end(),
                        [&](const char *ext) {
                          return std::strcmp(ext, name) == 0;
                        }) != deviceExtensions.end();
  };

  // Feature structures for the logical device
  vk::PhysicalDeviceFeatures2 features2{};
  features2.features.samplerAnisotropy = vk::True;
  features2.features.depthBiasClamp =
      coreSupported.depthBiasClamp ? vk::True : vk::False;
  // Reuqries enabling robust Buffer Access
  features2.features.robustBufferAccess = vk::True;
  if (coreSupported.shaderSampledImageArrayDynamicIndexing) {
    features2.features.shaderSampledImageArrayDynamicIndexing = vk::True;
  }

  vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
  timelineSemaphoreFeatures.timelineSemaphore = vk::True;

  vk::PhysicalDeviceVulkanMemoryModelFeatures memoryModelFeatures{};
  memoryModelFeatures.vulkanMemoryModel = vk::True;
  memoryModelFeatures.vulkanMemoryModelDeviceScope =
      memoryModelSupported.vulkanMemoryModelDeviceScope ? vk::True : vk::False;

  vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
  bufferDeviceAddressFeatures.bufferDeviceAddress = vk::True;

  vk::PhysicalDevice8BitStorageFeatures storage8BitFeatures{};
  storage8BitFeatures.storageBuffer8BitAccess =
      storage8BitSupported.storageBuffer8BitAccess ? vk::True : vk::False;

  vk::PhysicalDeviceVulkan11Features vulkan11Features{};
  if (vulkan11Supported.shaderDrawParameters) {
    vulkan11Features.shaderDrawParameters = vk::True;
  }

  vk::PhysicalDeviceVulkan13Features vulkan13Features{};
  vulkan13Features.dynamicRendering = vk::True;
  vulkan13Features.synchronization2 = vk::True;

  vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeaturesEnable{};
  descriptorIndexingEnabled = false;
  if (indexingFeaturesSupported.shaderSampledImageArrayNonUniformIndexing) {
    indexingFeaturesEnable.shaderSampledImageArrayNonUniformIndexing = vk::True;
    descriptorIndexingEnabled = true;
  }
  if (descriptorIndexingEnabled) {
    if (indexingFeaturesSupported.descriptorBindingPartiallyBound)
      indexingFeaturesEnable.descriptorBindingPartiallyBound = vk::True;
    if (indexingFeaturesSupported.descriptorBindingUpdateUnusedWhilePending)
      indexingFeaturesEnable.descriptorBindingUpdateUnusedWhilePending =
          vk::True;
    if (indexingFeaturesSupported
            .descriptorBindingSampledImageUpdateAfterBind) {
      indexingFeaturesEnable.descriptorBindingSampledImageUpdateAfterBind =
          vk::True;
      descriptorBindingSampledImageUpdateAfterBindEnabled = true;
    }
    if (indexingFeaturesSupported
            .descriptorBindingUniformBufferUpdateAfterBind) {
      indexingFeaturesEnable.descriptorBindingUniformBufferUpdateAfterBind =
          vk::True;
      descriptorBindingUniformBufferUpdateAfterBindEnabled = true;
    }
  }

  vk::PhysicalDeviceRobustness2FeaturesEXT robust2Enable{};

  bool hasRobust2 = hasExtension(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);
  if (hasRobust2) {
    if (robust2Supported.robustBufferAccess2)
      robust2Enable.robustBufferAccess2 = vk::True;
    if (robust2Supported.robustImageAccess2)
      robust2Enable.robustImageAccess2 = vk::True;
    if (robust2Supported.nullDescriptor)
      robust2Enable.nullDescriptor = vk::True;
  }
  robustness2Enabled = hasRobust2 && (robust2Enable.robustBufferAccess2 ||
                                      robust2Enable.robustImageAccess2 ||
                                      robust2Enable.nullDescriptor);

  vk::PhysicalDeviceDynamicRenderingLocalReadFeaturesKHR localReadEnable{};
  bool hasLocalRead =
      hasExtension(VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME);
  if (hasLocalRead && localReadSupported.dynamicRenderingLocalRead) {
    localReadEnable.dynamicRenderingLocalRead = vk::True;
  }
  dynamicRenderingLocalReadEnabled =
      hasLocalRead && localReadEnable.dynamicRenderingLocalRead;

  vk::PhysicalDeviceShaderTileImageFeaturesEXT tileImageEnable{};
  bool hasTileImage = hasExtension(VK_EXT_SHADER_TILE_IMAGE_EXTENSION_NAME);
  if (hasTileImage && tileImageSupported.shaderTileImageColorReadAccess) {
    tileImageEnable.shaderTileImageColorReadAccess = vk::True;
  }
  shaderTileImageEnabled =
      hasTileImage && tileImageEnable.shaderTileImageColorReadAccess;

  vk::PhysicalDeviceAccelerationStructureFeaturesKHR asFeaturesEnable{};
  bool hasAS = hasExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
  if (hasAS && accelerationStructureSupported.accelerationStructure) {
    asFeaturesEnable.accelerationStructure = vk::True;
  }
  accelerationStructureEnabled =
      hasAS && asFeaturesEnable.accelerationStructure;

  vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeaturesEnable{};
  bool hasRQ = hasExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME);
  if (hasRQ && rayQuerySupported.rayQuery) {
    rayQueryFeaturesEnable.rayQuery = vk::True;
  }
  rayQueryEnabled = hasRQ && rayQueryFeaturesEnable.rayQuery;

  // Build the pNext chain
  features2.pNext = &timelineSemaphoreFeatures;
  timelineSemaphoreFeatures.pNext = &memoryModelFeatures;
  memoryModelFeatures.pNext = &bufferDeviceAddressFeatures;
  bufferDeviceAddressFeatures.pNext = &storage8BitFeatures;
  storage8BitFeatures.pNext = &vulkan11Features;
  vulkan11Features.pNext = &vulkan13Features;

  void **tailNext = reinterpret_cast<void **>(&vulkan13Features.pNext);
  if (descriptorIndexingEnabled) {
    *tailNext = &indexingFeaturesEnable;
    tailNext = reinterpret_cast<void **>(&indexingFeaturesEnable.pNext);
  }
  if (hasRobust2) {
    *tailNext = &robust2Enable;
    tailNext = reinterpret_cast<void **>(&robust2Enable.pNext);
  }
#if !defined(PLATFORM_ANDROID)
  if (hasLocalRead) {
    *tailNext = &localReadEnable;
    tailNext = reinterpret_cast<void **>(&localReadEnable.pNext);
  }
  if (hasTileImage) {
    *tailNext = &tileImageEnable;
    tailNext = reinterpret_cast<void **>(&tileImageEnable.pNext);
  }
#endif
  if (hasAS) {
    *tailNext = &asFeaturesEnable;
    tailNext = reinterpret_cast<void **>(&asFeaturesEnable.pNext);
  }
  if (hasRQ) {
    *tailNext = &rayQueryFeaturesEnable;
    tailNext = reinterpret_cast<void **>(&rayQueryFeaturesEnable.pNext);
  }

  // Opacity micromap — VK_KHR_opacity_micromap (Course: Opacity Micromaps)
  // Also requires VK_KHR_device_address_commands for
  // vkCreateAccelerationStructure2KHR.
  auto hasOpacityMicromap =
      hasExtension(VK_KHR_OPACITY_MICROMAP_EXTENSION_NAME) &&
      hasExtension(VK_KHR_DEVICE_ADDRESS_COMMANDS_EXTENSION_NAME);
  vk::PhysicalDeviceOpacityMicromapFeaturesKHR opacityMicromapSupported{};
  vk::PhysicalDeviceOpacityMicromapFeaturesKHR opacityMicromapEnable{};
  if (hasOpacityMicromap) {
    auto featChain2 =
        physicalDevice
            .getFeatures2<vk::PhysicalDeviceFeatures2,
                          vk::PhysicalDeviceOpacityMicromapFeaturesKHR>();
    opacityMicromapSupported =
        featChain2.template get<vk::PhysicalDeviceOpacityMicromapFeaturesKHR>();
    if (opacityMicromapSupported.micromap) {
      opacityMicromapEnable.micromap = vk::True;
#ifdef ENABLE_COURSE_OPACITY_MICROMAPS
      opacityMicromapEnabled = true;
#endif
      *tailNext = &opacityMicromapEnable;
      tailNext = reinterpret_cast<void **>(&opacityMicromapEnable.pNext);
    }
  }

  // Record which features ended up enabled (for runtime decisions/tutorial
  // diagnostics)
  robustness2Enabled =
      hasRobust2 && (robust2Enable.robustBufferAccess2 == vk::True ||
                     robust2Enable.robustImageAccess2 == vk::True ||
                     robust2Enable.nullDescriptor == vk::True);
#if !defined(PLATFORM_ANDROID)
  dynamicRenderingLocalReadEnabled =
      hasLocalRead && (localReadEnable.dynamicRenderingLocalRead == vk::True);
  shaderTileImageEnabled =
      hasTileImage &&
      (tileImageEnable.shaderTileImageColorReadAccess == vk::True ||
       tileImageEnable.shaderTileImageDepthReadAccess == vk::True ||
       tileImageEnable.shaderTileImageStencilReadAccess == vk::True);
#else
  dynamicRenderingLocalReadEnabled = false;
  shaderTileImageEnabled = false;
#endif
  accelerationStructureEnabled =
      hasAS && (asFeaturesEnable.accelerationStructure == vk::True);
  rayQueryEnabled = hasRQ && (rayQueryFeaturesEnable.rayQuery == vk::True);

  // Create device info
  vk::DeviceCreateInfo createInfo{
      .pNext = &features2,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = nullptr};

  logInfo("Creating logical device...");
  device = vk::raii::Device(physicalDevice, createInfo);
  logInfo("Device created successfully");

  // Initialize the dispatcher with the device to load device-level functions
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

  // Get queue handles
  graphicsQueue =
      vk::raii::Queue(device, queueFamilyIndices.graphicsFamily.value(), 0);
  presentQueue =
      vk::raii::Queue(device, queueFamilyIndices.presentFamily.value(), 0);
  computeQueue =
      vk::raii::Queue(device, queueFamilyIndices.computeFamily.value(), 0);
  transferQueue =
      vk::raii::Queue(device, queueFamilyIndices.transferFamily.value(), 0);

  // Create global timeline semaphore for uploads early (needed before default
  // texture creation)
  vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo>
      timelineChain({}, {.semaphoreType = vk::SemaphoreType::eTimeline,
                         .initialValue = 0});
  uploadsTimeline =
      vk::raii::Semaphore(device, timelineChain.get<vk::SemaphoreCreateInfo>());
  uploadTimelineLastSubmitted.store(0, std::memory_order_relaxed);

  return true;
}

bool Renderer::CheckValidationLayerSupport() {
  // Get available layers
  std::vector<vk::LayerProperties> availableLayers =
      context.enumerateInstanceLayerProperties();

  // Check if all requested layers are available
  for (const char *layerName : validationLayers) {
    bool layerFound = false;
    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }
  return true;
}

namespace {

// Builds a single-mip, single-layer image barrier for sync2
// (vkCmdPipelineBarrier2). No queue-family ownership transfer.
vk::ImageMemoryBarrier2
MakeImageBarrier(vk::Image image, vk::ImageAspectFlags aspectMask,
                 vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                 vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess,
                 vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess) {
  vk::ImageMemoryBarrier2 barrier{};
  barrier.srcStageMask = srcStage;
  barrier.srcAccessMask = srcAccess;
  barrier.dstStageMask = dstStage;
  barrier.dstAccessMask = dstAccess;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = aspectMask;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  return barrier;
}

} // namespace

} // namespace SimpleEngineVK
