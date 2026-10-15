#define VMA_IMPLEMENTATION
#include "VMAImpl.hpp"

MemoryPool::MemoryPool(const vk::raii::Device &device,
                       const vk::raii::PhysicalDevice &physicalDevice,
                       const vk::raii::Instance &instance)
    : device{device}, physicalDevice{physicalDevice}, instance{instance} {}

bool MemoryPool::CreateVmaAllocator() {
  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
  allocatorCreateInfo.physicalDevice = *physicalDevice;
  allocatorCreateInfo.device = *device;
  allocatorCreateInfo.instance = *instance;
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  if (vmaCreateAllocator(&allocatorCreateInfo, &allocator) == VK_SUCCESS) {
    return true;
  }
  return false;
}

std::pair<vk::raii::Buffer, VmaAllocation> MemoryPool::CreateBuffer(
    const vk::DeviceSize size, const vk::BufferUsageFlagBits usage,
    const vk::Flags<vk::MemoryPropertyFlagBits> properties) {

  // Determine a pool type based on usage and properties
  PoolType poolType = PoolType::VERTEX_BUFFER;

  // Check for host-visible requirements first (for instance buffers and
  // staging)
  if (properties & vk::MemoryPropertyFlagBits::eHostVisible) {
    poolType = PoolType::STAGING_BUFFER;
  } else if (usage & vk::BufferUsageFlagBits::eVertexBuffer) {
    poolType = PoolType::VERTEX_BUFFER;
  } else if (usage & vk::BufferUsageFlagBits::eIndexBuffer) {
    poolType = PoolType::INDEX_BUFFER;
  } else if (usage & vk::BufferUsageFlagBits::eUniformBuffer) {
    poolType = PoolType::UNIFORM_BUFFER;
  }

  vk::BufferCreateInfo bufferInfo = {
      .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};

  vk::raii::Buffer buffer(device, bufferInfo);
  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  VkBuffer act_buffer = *buffer;

  vmaCreateBuffer(allocator, &*bufferInfo, &allocInfo, &act_buffer, &allocation,
                  nullptr);
  return {std::move(buffer), std::move(allocation)};
}
