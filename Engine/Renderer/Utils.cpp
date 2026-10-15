#include "../Logger.hpp"
#include "Renderer.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <fstream>
#include <set>
#include <vulkan/vulkan_raii.hpp>

namespace SimpleEngineVK {

QueueFamilyIndices
Renderer::findQueueFamilies(const vk::raii::PhysicalDevice &device) {
  QueueFamilyIndices indices;

  std::vector<vk::QueueFamilyProperties> queueFamilies =
      device.getQueueFamilyProperties();

  // Find queue families that support graphics, compute, present, and
  // (optionally) a dedicated transfer queue
  for (uint32_t i = 0; i < queueFamilies.size(); i++) {
    const auto &qf = queueFamilies[i];
    // Check for graphics support
    if ((qf.queueFlags & vk::QueueFlagBits::eGraphics) &&
        !indices.graphicsFamily.has_value()) {
      indices.graphicsFamily = i;
    }
    // Check for compute support
    if ((qf.queueFlags & vk::QueueFlagBits::eCompute) &&
        !indices.computeFamily.has_value()) {
      indices.computeFamily = i;
    }
    // Check for present support
    if (!indices.presentFamily.has_value() &&
        device.getSurfaceSupportKHR(i, *surface)) {
      indices.presentFamily = i;
    }
    // Prefer a dedicated transfer queue (transfer bit set, but NOT graphics) if
    // available
    if ((qf.queueFlags & vk::QueueFlagBits::eTransfer) &&
        !(qf.queueFlags & vk::QueueFlagBits::eGraphics)) {
      if (!indices.transferFamily.has_value()) {
        indices.transferFamily = i;
      }
    }
    // If all required queue families are found, we can still continue to try
    // find a dedicated transfer queue
    if (indices.isComplete() && indices.transferFamily.has_value()) {
      // Found everything including dedicated transfer
      break;
    }
  }

  // Fallback: if no dedicated transfer queue, reuse graphics queue for transfer
  if (!indices.transferFamily.has_value() &&
      indices.graphicsFamily.has_value()) {
    indices.transferFamily = indices.graphicsFamily;
  }

  return indices;
}

// Check device extension support
bool Renderer::checkDeviceExtensionSupport(vk::raii::PhysicalDevice &device) {
  auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();

  // Check if all required extensions are supported
  std::set<std::string> requiredExtensionsSet(requiredDeviceExtensions.begin(),
                                              requiredDeviceExtensions.end());

  for (const auto &extension : availableDeviceExtensions) {
    requiredExtensionsSet.erase(extension.extensionName);
  }

  // Print missing required extensions
  if (!requiredExtensionsSet.empty()) {
    std::cout << "Missing required extensions:" << std::endl;
    for (const auto &extension : requiredExtensionsSet) {
      std::cout << "  " << extension << std::endl;
    }
    return false;
  }

  return true;
}

// Query swap chain support
SwapChainSupportDetails
Renderer::querySwapChainSupport(const vk::raii::PhysicalDevice &device) {
  SwapChainSupportDetails details;

  // Get surface capabilities
  details.capabilities = device.getSurfaceCapabilitiesKHR(*surface);

  // Get surface formats
  details.formats = device.getSurfaceFormatsKHR(*surface);

  // Get present modes
  details.presentModes = device.getSurfacePresentModesKHR(*surface);

  return details;
}

// Choose swap surface format
vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
  // Look for SRGB format
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return availableFormat;
    }
  }

  // If not found, return first available format
  return availableFormats[0];
}

// Choose swap present mode
vk::PresentModeKHR Renderer::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes) {
  // Look for mailbox mode (triple buffering)
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      return availablePresentMode;
    }
  }

  // If not found, return FIFO mode (guaranteed to be available)
  return vk::PresentModeKHR::eFifo;
}

// Choose swap extent
vk::Extent2D
Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    logInfo("Renderer: Using surface currentExtent: %dx%d");
    std::cout << capabilities.currentExtent.width << " "
              << capabilities.currentExtent.height << "\n";
    return capabilities.currentExtent;
  } else {
    // Get framebuffer size
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    logInfo(
        "Renderer: surface extent is undefined. Using platform window size: "
        "%dx%d");
    std::cout << "Widht : " << width << ""
              << "Height" << height << "\n";

    // Create extent
    vk::Extent2D actualExtent = {static_cast<uint32_t>(width),
                                 static_cast<uint32_t>(height)};

    // Clamp to min/max extent
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    logInfo("Renderer: Clamped extent: ", actualExtent.width, "x",
            actualExtent.height);

    return actualExtent;
  }
}

// Wait for device to be idle
void Renderer::WaitIdle() {
  // 1. Wait for all in-flight fences safely first
  std::vector<vk::Fence> allFences;
  allFences.reserve(inFlightFences.size());
  for (const auto &fence : inFlightFences) {
    if (*fence) {
      allFences.push_back(*fence);
    }
  }
  if (!allFences.empty()) {
    (void)WaitForFencesSafe(allFences, VK_TRUE);
  }

  // 2. Also wait for uploads timeline semaphore if it exists
  if (*uploadsTimeline) {
    uint64_t target =
        uploadTimelineLastSubmitted.load(std::memory_order_relaxed);
    while (true) {
      vk::SemaphoreWaitInfo waitInfo{};
      waitInfo.semaphoreCount = 1;
      waitInfo.pSemaphores = &*uploadsTimeline;
      waitInfo.pValues = &target;

      vk::Result r = device.waitSemaphores(waitInfo, 100'000'000ULL); // 100ms
      if (r == vk::Result::eSuccess)
        break;
      // NOTE : to fix later
      // if (r == vk::Result::eTimeout) {
      //  lastFrameUpdateTime.store(std::chrono::steady_clock::now(),
      //                            std::memory_order_relaxed);
      //  continue;
      //}
      break; // Other error
    }
  }

  // 3. Final blocking wait to ensure absolute idle
  // External synchronization: ensure no queue submits/presents overlap a full
  // device idle. This is required for VVL cleanliness when other threads may
  // hold or use queues.
  // std::lock_guard<std::mutex> lock(queueMutex);
  device.waitIdle();
}
vk::Result Renderer::WaitForFencesSafe(const std::vector<vk::Fence> &fences,
                                       vk::Bool32 waitAll, uint64_t timeoutNs) {
  if (fences.empty())
    return vk::Result::eSuccess;

  vk::Result r;
  do {
    r = device.waitForFences(fences, waitAll, timeoutNs);
  } while (r == vk::Result::eTimeout);
  return r;
}

vk::Result Renderer::WaitForFencesSafe(vk::Fence fence, vk::Bool32 waitAll,
                                       uint64_t timeoutNs) {
  return WaitForFencesSafe(std::vector<vk::Fence>{fence}, waitAll, timeoutNs);
}

std::vector<char> Renderer::readFile(const std::string &filename) {
  // Open file at end to get size
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    logError("Failed to open file :", filename)    ;
  }

  // Get file size
  size_t fileSize = file.tellg();
  std::vector<char> buffer(fileSize);

  // Go back to beginning of file and read data
  file.seekg(0);
  file.read(buffer.data(), fileSize);

  // Close file
  file.close();

  return buffer;
}

// Create shader module
vk::raii::ShaderModule
Renderer::CreateShaderModule(const std::vector<char> &code) {
  // Ensure 4-byte alignment for pCode

  std::vector<uint32_t> alignedCode(code.size() / 4);
  std::memcpy(alignedCode.data(), code.data(), code.size());

  // Create shader module
  vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size(),
                                        .pCode = alignedCode.data()};

  return vk::raii::ShaderModule(device, createInfo);
}

// Find supported format
vk::Format
Renderer::FindSupportedFormat(const std::vector<vk::Format> &candidates,
                              vk::ImageTiling tiling,
                              vk::FormatFeatureFlags features) {
  for (vk::Format format : candidates) {
    vk::FormatProperties props = physicalDevice.getFormatProperties(format);

    if (tiling == vk::ImageTiling::eLinear &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  logError("Failed to find suitable format");
  return vk::Format::eUndefined;
}

// Find depth format
vk::Format Renderer::FindDepthFormat() {
  vk::Format depthFormat =
      FindSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
                           vk::Format::eD24UnormS8Uint},
                          vk::ImageTiling::eOptimal,
                          vk::FormatFeatureFlagBits::eDepthStencilAttachment);
  std::cout << "Found depth format: " << static_cast<int>(depthFormat)
            << std::endl;
  return depthFormat;
}

// Check if format has stencil component
bool Renderer::HasStencilComponent(vk::Format format) {
  return format == vk::Format::eD32SfloatS8Uint ||
         format == vk::Format::eD24UnormS8Uint;
}

// Find a memory type index that is allowed for the resource (typeFilter is
// VkMemoryRequirements::memoryTypeBits) and has all the requested properties.
// uint32_t Renderer::FindMemoryType(uint32_t typeFilter,
//                                   vk::MemoryPropertyFlags properties) {
//   vk::PhysicalDeviceMemoryProperties memProperties =
//       physicalDevice.getMemoryProperties();

//   for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
//     if ((typeFilter & (1u << i)) &&
//         (memProperties.memoryTypes[i].propertyFlags & properties) ==
//             properties) {
//       return i;
//     }
//   }
//   assert("Failed to find ")

// }

} // namespace SimpleEngineVK
