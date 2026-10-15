#pragma once

#include "vulkan/vulkan_raii.hpp"


class VulkanTexture {
public:
  VulkanTexture();  
  vk::raii::Image m_Image;
  vk::raii::DeviceMemory m_Memory;
  vk::raii::Sampler m_Sampler;
  vk::raii::ImageView m_ImageView;

  void Destroy();  
};
