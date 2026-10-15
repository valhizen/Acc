#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include "../Logger.hpp"
#include "Renderer.hpp"

namespace SimpleEngineVK {
// Create the depth buffer (image + dedicated memory + view), sized to the
// swapchain extent.
bool Renderer::CreateDepthResources() {
  depthImageFormat = FindDepthFormat();

  vk::ImageCreateInfo imageInfo{
      .imageType = vk::ImageType::e2D,
      .format = depthImageFormat,
      .extent = {swapChainExtent.width, swapChainExtent.height, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = vk::SampleCountFlagBits::e1,
      .tiling = vk::ImageTiling::eOptimal,
      .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
      .sharingMode = vk::SharingMode::eExclusive,
      .initialLayout = vk::ImageLayout::eUndefined};

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

  VkImageCreateInfo vkImageInfo = imageInfo;
  VkImage image = VK_NULL_HANDLE;
  if (vmaCreateImage(memoryPool->allocator, &vkImageInfo, &allocInfo, &image,
                     &depthAllocation, nullptr) != VK_SUCCESS) {
    logError("Failed to create depth image");
    return false;
  }

  depthImage = vk::Image(image);

  vk::ImageViewCreateInfo viewInfo{
      .image = depthImage,
      .viewType = vk::ImageViewType::e2D,
      .format = depthImageFormat,
      .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eDepth,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};
  depthImageView = vk::raii::ImageView(device, viewInfo);

  return true;
}

bool Renderer::CreateDescriptorPool() {

  const uint32_t maxEntities = 20000;
  const uint32_t maxDescriptorsSet = MAX_FRAMES_IN_FLIGHT * maxEntities * 2;

  const uint32_t uboDescriptor = maxDescriptorsSet;

  std::array<vk::DescriptorPoolSize, 1> poolSizes = {
      vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer,
                             .descriptorCount = uboDescriptor}};

  vk::DescriptorPoolCreateFlags poolFlags =
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
  if (descriptorIndexingEnabled) {
    poolFlags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
  }

  vk::DescriptorPoolCreateInfo poolInfo = {
      .flags = poolFlags,
      .maxSets = maxDescriptorsSet,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()};

  descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
  return true;
}

bool Renderer::CreateDescriptorSets() {

  // Only initialize the current frame's descriptor set at runtime to avoid
  // updating descriptor sets that may be in use by pending command buffers.
  // Other frames will be initialized at their own safe points.

  size_t startIndex = static_cast<size_t>(currentFrame);
  size_t endIndex = startIndex + 1;

  for (size_t i = startIndex; i < endIndex; i++) {
  }
  return true;
}

bool Renderer::CreateUniformBuffers() {
  vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    auto [buffer, bufferMem] = memoryPool->CreateBuffer(
        bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);

    uniformBuffers.emplace_back(std::move(buffer));
    uniformBuffersMemory.emplace_back(std::move(bufferMem));
    uniformBuffersMapped.emplace_back();
    // uniformBuffersMemory.back().mapMemory(0, bufferSize)); // 

  }
  return true;
}

} // namespace SimpleEngineVK
