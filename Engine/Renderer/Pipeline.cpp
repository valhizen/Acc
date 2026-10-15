#include "Renderer.hpp"
#include <iostream>

namespace SimpleEngineVK {

// Create a descriptor set layout
bool Renderer::CreateDescriptorSetLayout() {
  // Create binding for a uniform buffer
  vk::DescriptorSetLayoutBinding uboLayoutBinding{
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags =
          vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      .pImmutableSamplers = nullptr};

  // Create binding for texture sampler
  vk::DescriptorSetLayoutBinding samplerLayoutBinding{
      .binding = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment,
      .pImmutableSamplers = nullptr};

  // Create a descriptor set layout
  std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
      uboLayoutBinding, samplerLayoutBinding};

  // Descriptor indexing: set per-binding flags for UPDATE_AFTER_BIND if
  // enabled
  vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
  std::array<vk::DescriptorBindingFlags, 2> bindingFlags{};
  if (descriptorIndexingEnabled) {
    if (descriptorBindingUniformBufferUpdateAfterBindEnabled) {
      bindingFlags[0] =
          vk::DescriptorBindingFlagBits::eUpdateAfterBind |
          vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
    }
    if (descriptorBindingSampledImageUpdateAfterBindEnabled) {
      bindingFlags[1] =
          vk::DescriptorBindingFlagBits::eUpdateAfterBind |
          vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
    }
    bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
    bindingFlagsInfo.pBindingFlags = bindingFlags.data();
  }

  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  if (descriptorIndexingEnabled) {
    layoutInfo.flags |=
        vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
    layoutInfo.pNext = &bindingFlagsInfo;
  }

  descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
  return true;
}

bool Renderer::CreateGraphicsPipeline() {

  // Read shader code
  auto shaderCode = readFile("../../../src/Shaders/default.spv");

  // Create shader module
  vk::raii::ShaderModule shaderModule = CreateShaderModule(shaderCode);

  // Create shader stage info
  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
  vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
  vertShaderStageInfo.module = *shaderModule;
  vertShaderStageInfo.pName = "VSMain";

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
  fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
  fragShaderStageInfo.module = *shaderModule;
  fragShaderStageInfo.pName = "FSMain";

  vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

  //// Create vertex input info with instancing support
  // auto vertexBindingDescription = Vertex::getBindingDescription();
  // auto instanceBindingDescription = InstanceData::getBindingDescription();
  // std::array<vk::VertexInputBindingDescription, 2> bindingDescriptions = {
  //     vertexBindingDescription, instanceBindingDescription};

  // auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
  // auto instanceAttributeDescriptions =
  //     InstanceData::getAttributeDescriptions();

  //// Combine all attribute descriptions (no duplicates)
  // std::vector<vk::VertexInputAttributeDescription>
  // allAttributeDescriptions;
  // allAttributeDescriptions.insert(allAttributeDescriptions.end(),
  //                                 vertexAttributeDescriptions.begin(),
  //                                 vertexAttributeDescriptions.end());
  // allAttributeDescriptions.insert(allAttributeDescriptions.end(),
  //                                 instanceAttributeDescriptions.begin(),
  //                                 instanceAttributeDescriptions.end());

  //// Note: materialIndex attribute (Location 11) is not used by current
  //// shaders

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

  //// Create input assembly info
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
      .topology = vk::PrimitiveTopology::eTriangleList,
      .primitiveRestartEnable = VK_FALSE};

  // Create viewport state info
  vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1,
                                                    .scissorCount = 1};

  // Create rasterization state info
  vk::PipelineRasterizationStateCreateInfo rasterizer{
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eNone,
      .frontFace = vk::FrontFace::eCounterClockwise,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f};

  // Create multisample state info
  vk::PipelineMultisampleStateCreateInfo multisampling{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = VK_FALSE};

  // Create depth stencil state info
  vk::PipelineDepthStencilStateCreateInfo depthStencil{
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      // Use LessOrEqual so that the main shading pass works after a depth
      // pre-pass
      .depthCompareOp = vk::CompareOp::eLessOrEqual,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE};

  // Create a color blend attachment state
  vk::PipelineColorBlendAttachmentState colorBlendAttachment{
      .blendEnable = VK_FALSE,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

  // Create color blend state info
  vk::PipelineColorBlendStateCreateInfo colorBlending{
      .logicOpEnable = VK_FALSE,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment};

  // Create dynamic state info
  std::vector dynamicStates = {vk::DynamicState::eViewport,
                               vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dynamicState{
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data()};

  // Create pipeline layout
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
      // .setLayoutCount = 1,
      // .pSetLayouts = &*descriptorSetLayout,
      // .pushConstantRangeCount = 0,
      // .pPushConstantRanges = nullptr
  };

  pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

  // Create pipeline rendering info
  vk::Format depthFormat = FindDepthFormat();
  std::cout << "Creating main graphics pipeline with depth format: "
            << static_cast<int>(depthFormat) << std::endl;

  vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
  pipelineRenderingInfo.sType = vk::StructureType::ePipelineRenderingCreateInfo;
  pipelineRenderingInfo.colorAttachmentCount = 1;
  pipelineRenderingInfo.pColorAttachmentFormats = &swapChainImageFormat;
  pipelineRenderingInfo.depthAttachmentFormat = depthFormat;
  pipelineRenderingInfo.stencilAttachmentFormat = vk::Format::eUndefined;

  // Create the graphics pipeline
  vk::PipelineRasterizationStateCreateInfo rasterizerBack = rasterizer;
  // Disable back-face culling for opaque PBR to avoid disappearing geometry
  // when instance/model transforms flip winding (ensures PASS 1 actually
  // shades pixels)
  rasterizerBack.cullMode = vk::CullModeFlagBits::eNone;

  vk::GraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
  pipelineInfo.pNext = &pipelineRenderingInfo;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizerBack;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = *pipelineLayout;

  graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
  return true;
}

} // namespace SimpleEngineVK
