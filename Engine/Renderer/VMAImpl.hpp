#pragma once

#include <vma/vk_mem_alloc.h> 
 
 
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS 
#include <vulkan/vulkan_raii.hpp> 
class MemoryPool { 
public: 
  enum class PoolType { 
    VERTEX_BUFFER,  // Device-local memory for vertex data 
    INDEX_BUFFER,   // Device-local memory for index data 
    UNIFORM_BUFFER, // Host-visible memory for uniform data 
    STAGING_BUFFER, // Host-visible memory for staging operations 
    TEXTURE_IMAGE   // Device-local memory for texture images 
  }; 
 
  MemoryPool(const vk::raii::Device &device,
             const vk::raii::PhysicalDevice &physicalDevice,
             const vk::raii::Instance &instance);
  // std::pair<vk::raii::Buffer, VmaAllocation allocation> CreateBuffe// r(); //
  std::pair<vk::raii::Buffer, VmaAllocation>
  CreateBuffer(const vk::DeviceSize size, const vk::BufferUsageFlagBits flags,
               const vk::Flags<vk::MemoryPropertyFlagBits> properties);

  bool CreateVmaAllocator();

  VmaAllocator allocator{nullptr};
  VmaAllocation allocation;

private:
  const vk::raii::Device &device;
  const vk::raii::PhysicalDevice &physicalDevice;
  const vk::raii::Instance &instance;
};
