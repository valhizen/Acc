#include "../Logger.hpp"
#include "Renderer.hpp"

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>


namespace SimpleEngineVK {

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

bool Renderer::CreateSwapchain() {

  // Query swapchain support detail
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice);

  // Choose swap surface format, present mode, and extent
  vk::SurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  vk::PresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  // Choose image count
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  // Choose preTransform. On Android, eIdentity is preferred if supported to
  // let the system handle rotation.
  vk::SurfaceTransformFlagBitsKHR preTransform;
  if (swapChainSupport.capabilities.supportedTransforms &
      vk::SurfaceTransformFlagBitsKHR::eIdentity) {
    preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  } else {
    preTransform = swapChainSupport.capabilities.currentTransform;
  }

  // Create swap chain info
  vk::SwapchainCreateInfoKHR createInfo{
      .surface = *surface,
      .minImageCount = imageCount,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment |
                    vk::ImageUsageFlagBits::eTransferDst,
      .preTransform = preTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = presentMode,
      .clipped = VK_TRUE,
      .oldSwapchain = nullptr};

  // Find queue families
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  std::array<uint32_t, 2> queueFamilyIndicesLoc = {
      indices.graphicsFamily.value(), indices.presentFamily.value()};

  // Set sharing mode
  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount =
        static_cast<uint32_t>(queueFamilyIndicesLoc.size());
    createInfo.pQueueFamilyIndices = queueFamilyIndicesLoc.data();
  } else {
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  // Create swap chain
  swapChain = vk::raii::SwapchainKHR(device, createInfo);

  // Get swap chain images
  swapChainImages = swapChain.getImages();

  // Swapchain images start in UNDEFINED layout; track per-image layout for
  // correct barriers.
  swapChainImageLayouts.assign(swapChainImages.size(),
                               vk::ImageLayout::eUndefined);

  // Store swap chain format and extent
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;

  return true;
}

bool Renderer::CreateImageViews() {

  // Resize image views vector
  swapChainImageViews.clear();
  swapChainImageViews.reserve(swapChainImages.size());

  // Create image view info template (image will be set per iteration)
  vk::ImageViewCreateInfo createInfo = {
      .viewType = vk::ImageViewType::e2D,
      .format = swapChainImageFormat,
      .components = {.r = vk::ComponentSwizzle::eIdentity,
                     .g = vk::ComponentSwizzle::eIdentity,
                     .b = vk::ComponentSwizzle::eIdentity,
                     .a = vk::ComponentSwizzle::eIdentity

      },
      .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}

  };

  // Create image view for each swap chain image
  for (const auto &image : swapChainImages) {
    createInfo.image = image;
    swapChainImageViews.emplace_back(device, createInfo);
  }
  return true;
}

bool Renderer::SetupDynamicRendering() {

  // Create color attachment
  colorAttachments = {vk::RenderingAttachmentInfo{
      .imageView = *depthImageView,
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearColorValue(
          std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}) // Black default

  }};

  // Create depth attachment
  depthAttachment = vk::RenderingAttachmentInfo{
      .imageView = *depthImageView,
      .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = vk::ClearDepthStencilValue(1.0f, 0)};

  // Create rendering info
  renderingInfo = vk::RenderingInfo{
      .renderArea = vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent),
      .layerCount = 1,
      .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
      .pColorAttachments = colorAttachments.data(),
      .pDepthAttachment = &depthAttachment};

  return true;
}

// Create command pool
bool Renderer::CreateCommandPool() {
  QueueFamilyIndices queueFamilyIndicesLoc = findQueueFamilies(physicalDevice);

  // Create command pool info
  vk::CommandPoolCreateInfo poolInfo{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queueFamilyIndicesLoc.graphicsFamily.value()};

  // Create command pool
  commandPool = vk::raii::CommandPool(device, poolInfo);

  return true;
}

bool Renderer::CreateCommandBuffers() {
  commandBuffers.clear();
  commandBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

  vk::CommandBufferAllocateInfo allocInfo = {
      .commandPool = *commandPool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)};

  commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
  return true;
}

bool Renderer::CreateSyncObjects() {

  // Resize semaphores and fences vectors
  imageAvailableSemaphores.clear();
  renderFinishedSemaphores.clear();
  inFlightFences.clear();

  // Semaphores per swapchain image (indexed by imageIndex from
  // acquireNextImage)
  // The presentation engine holds semaphores until the image is re-acquired,
  // so we need one semaphore per swapchain image to avoid reuse conflicts.
  // See Vulkan spec:
  // https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
  const auto semaphoreCount = static_cast<uint32_t>(swapChainImages.size());
  imageAvailableSemaphores.reserve(semaphoreCount);
  renderFinishedSemaphores.reserve(semaphoreCount);

  // Fences per frame-in-flight for CPU-GPU synchronization (indexed by
  // currentFrame)
  inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

  // Create semaphore info
  vk::SemaphoreCreateInfo semaphoreInfo{};

  // Create semaphores per swapchain image (indexed by imageIndex for
  // presentation sync)
  for (uint32_t i = 0; i < semaphoreCount; i++) {
    imageAvailableSemaphores.emplace_back(device, semaphoreInfo);
    renderFinishedSemaphores.emplace_back(device, semaphoreInfo);
  }

  // Create fences per frame-in-flight (indexed by currentFrame for CPU-GPU
  // pacing)
  vk::FenceCreateInfo fenceInfo{.flags = vk::FenceCreateFlagBits::eSignaled};
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    inFlightFences.emplace_back(device, fenceInfo);
  }

  // Ensure uploads timeline semaphore exists (created early in
  // createLogicalDevice) No action needed here unless reinitializing after
  // swapchain recreation.
  return true;
}

// Cleanup Swapchain
void Renderer::CleanupSwapChain() {

  // Clean up depth resources
  depthImageView = vk::raii::ImageView(nullptr);
  vmaDestroyImage(memoryPool->allocator, depthImage, depthAllocation);
  depthImage = nullptr;
  depthAllocation = nullptr;

  // Clean up swap chain image views
  swapChainImageViews.clear();

  // Clean up pipelines
  graphicsPipeline = vk::raii::Pipeline(nullptr);

  // Clean up pipeline layouts
  pipelineLayout = vk::raii::PipelineLayout(nullptr);

  // Clean up sync objects (they need to be recreated with new swap chain image
  // count)
  imageAvailableSemaphores.clear();
  renderFinishedSemaphores.clear();
  inFlightFences.clear();

  // Finally the swapchain itself.
  swapChain = vk::raii::SwapchainKHR(nullptr);
}

// Rebuild everything that depends on the swapchain (window resized, surface
// went out of date, ...). Returns false if anything failed to be recreated.
void Renderer::ReCreateSwapchain() {
  // A minimized window has a 0x0 framebuffer and a 0x0 swapchain is invalid,
  // so block here until the window has a real size again.
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwWaitEvents();
    glfwGetFramebufferSize(window, &width, &height);
  }

  // Nothing may still be using the old resources.
  WaitIdle();

  CleanupSwapChain();

  CreateSwapchain();
  CreateImageViews();

  if (!CreateDepthResources()) {
    logError("Failed to Create Depth Resource");
  }
  if (!SetupDynamicRendering()) {
    logError("Failed to setup dynamic rendering");
  }

  CreateSyncObjects();
  CreateGraphicsPipeline();

  commandBuffers.clear();
  CreateCommandBuffers();
  currentFrame = 0;
  logInfo("Swapchain recreated: ", swapChainExtent.width, "x",
          swapChainExtent.height);
}

void Renderer::Render() {

  vk::Result fenceResult =

      WaitForFencesSafe(*inFlightFences[currentFrame], VK_TRUE);

  if (fenceResult != vk::Result::eSuccess) {

    std::cerr << "Error: Failed to wait for in-flight fence: "

              << vk::to_string(fenceResult) << std::endl;

    return;
  }

  const uint32_t acquireSemaphoreIndex =

      currentFrame % static_cast<uint32_t>(imageAvailableSemaphores.size());

  uint32_t imageIndex = 0;

  vk::Result acquireResultCode = vk::Result::eSuccess;

  // Helper overloads to normalize acquireNextImage return across Vulkan-Hpp

  // versions

  auto extractAcquire = [](auto const &ret, vk::Result &code, uint32_t &idx) {
    using RetT = std::decay_t<decltype(ret)>;

    if constexpr (std::is_same_v<RetT, vk::ResultValue<uint32_t>>) {

      code = ret.result;

      idx = ret.value;

    } else {

      // Assume older std::pair<vk::Result, uint32_t>

      code = ret.first;

      idx = ret.second;
    }
  };

  auto acquireRet = swapChain.acquireNextImage(

      UINT64_MAX, *imageAvailableSemaphores[acquireSemaphoreIndex]);

  extractAcquire(acquireRet, acquireResultCode, imageIndex);

  if (acquireResultCode == vk::Result::eErrorOutOfDateKHR) {

    // Swapchain no longer matches the surface (e.g. window resized). No image

    // was acquired, so there is nothing to render this frame.

    ReCreateSwapchain();

    return;
  }

  if (acquireResultCode != vk::Result::eSuccess &&

      acquireResultCode != vk::Result::eSuboptimalKHR) {

    logError("Failed to acquire swapchain image: ",

             vk::to_string(acquireResultCode));

    return;
  }

  bool needsRecreate = (acquireResultCode == vk::Result::eSuboptimalKHR);

  device.resetFences(*inFlightFences[currentFrame]);

  auto &cmd = commandBuffers[currentFrame];

  cmd.reset();

  cmd.begin(vk::CommandBufferBeginInfo{

      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

  const vk::Viewport viewport(

      0.0f, 0.0f, static_cast<float>(swapChainExtent.width),

      static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);

  const vk::Rect2D scissor({0, 0}, swapChainExtent);

  const vk::ImageAspectFlags depthAspect =

      HasStencilComponent(depthImageFormat)

          ? (vk::ImageAspectFlagBits::eDepth |

             vk::ImageAspectFlagBits::eStencil)

          : vk::ImageAspectFlags(vk::ImageAspectFlagBits::eDepth);

  std::array<vk::ImageMemoryBarrier2, 2> beginBarriers = {

      MakeImageBarrier(swapChainImages[imageIndex],

                       vk::ImageAspectFlagBits::eColor,

                       swapChainImageLayouts[imageIndex],

                       vk::ImageLayout::eColorAttachmentOptimal,

                       vk::PipelineStageFlagBits2::eColorAttachmentOutput,

                       vk::AccessFlagBits2::eNone,

                       vk::PipelineStageFlagBits2::eColorAttachmentOutput,

                       vk::AccessFlagBits2::eColorAttachmentWrite),

      MakeImageBarrier(depthImage, depthAspect, vk::ImageLayout::eUndefined,

                       vk::ImageLayout::eDepthStencilAttachmentOptimal,

                       vk::PipelineStageFlagBits2::eEarlyFragmentTests |

                           vk::PipelineStageFlagBits2::eLateFragmentTests,

                       vk::AccessFlagBits2::eDepthStencilAttachmentWrite,

                       vk::PipelineStageFlagBits2::eEarlyFragmentTests |

                           vk::PipelineStageFlagBits2::eLateFragmentTests,

                       vk::AccessFlagBits2::eDepthStencilAttachmentRead |

                           vk::AccessFlagBits2::eDepthStencilAttachmentWrite)};

  cmd.pipelineBarrier2(vk::DependencyInfo{

      .imageMemoryBarrierCount = static_cast<uint32_t>(beginBarriers.size()),

      .pImageMemoryBarriers = beginBarriers.data()});

  colorAttachments[0].imageView = *swapChainImageViews[imageIndex];

  cmd.beginRendering(renderingInfo);

  cmd.setViewport(0, viewport);

  cmd.setScissor(0, scissor);

  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

  cmd.draw(3, 1, 0, 0);

  cmd.endRendering();

  vk::ImageMemoryBarrier2 presentBarrier = MakeImageBarrier(

      swapChainImages[imageIndex], vk::ImageAspectFlagBits::eColor,

      vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,

      vk::PipelineStageFlagBits2::eColorAttachmentOutput,

      vk::AccessFlagBits2::eColorAttachmentWrite,

      vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone);

  cmd.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1,

                                          .pImageMemoryBarriers =

                                              &presentBarrier});

  swapChainImageLayouts[imageIndex] = vk::ImageLayout::ePresentSrcKHR;

  {

    cmd.end();

    const vk::PipelineStageFlags waitStage =

        vk::PipelineStageFlagBits::eColorAttachmentOutput;

    const vk::Semaphore waitSemaphore =

        *imageAvailableSemaphores[acquireSemaphoreIndex];

    const vk::Semaphore signalSemaphore = *renderFinishedSemaphores[imageIndex];

    const vk::CommandBuffer cmdHandle = *cmd;

    vk::SubmitInfo submitInfo{.waitSemaphoreCount = 1,

                              .pWaitSemaphores = &waitSemaphore,

                              .pWaitDstStageMask = &waitStage,

                              .commandBufferCount = 1,

                              .pCommandBuffers = &cmdHandle,

                              .signalSemaphoreCount = 1,

                              .pSignalSemaphores = &signalSemaphore};

    graphicsQueue.submit(submitInfo, *inFlightFences[currentFrame]);

    const vk::SwapchainKHR swapchainHandle = *swapChain;

    vk::PresentInfoKHR presentInfo{.waitSemaphoreCount = 1,

                                   .pWaitSemaphores = &signalSemaphore,

                                   .swapchainCount = 1,

                                   .pSwapchains = &swapchainHandle,

                                   .pImageIndices = &imageIndex};

    vk::Result presentResult = vk::Result::eSuccess;

    try {

      presentResult = presentQueue.presentKHR(presentInfo);

    } catch (const std::exception &) {

      needsRecreate = true;
    }
    if (needsRecreate) {

      // Must be the last thing we do with this frame's sync objects: recreation

      // destroys and rebuilds all semaphores and fences.

      ReCreateSwapchain();
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }
}

} // namespace SimpleEngineVK
