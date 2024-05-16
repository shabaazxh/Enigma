#include "UIPass.h"

namespace Enigma {
UIPass::UIPass(const VulkanContext& context, const VulkanWindow& window) : context{context}, window{window} {
    m_width = window.swapchainExtent.width;
    m_height = window.swapchainExtent.height;

    CreateRenderPass();
    CreateBlood();
}

UIPass::~UIPass() {
    vkDestroyRenderPass(context.device, m_RenderPass, 0);
}
void UIPass::Execute(VkCommandBuffer cmd) {
    VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpBegin.renderPass = m_RenderPass;
    rpBegin.framebuffer = window.swapchainFramebuffers[Enigma::currentFrame];  // this should be image index
    rpBegin.renderArea.extent = window.swapchainExtent;

    VkClearValue clearValues[1];
    clearValues[0].color = {{0.0f, 0.0f, 0.5f, 1.0f}};
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = clearValues;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_width;
    viewport.height = (float)m_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {m_width, m_height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
    DrawBlood(cmd);
    vkCmdEndRenderPass(cmd);
}
void UIPass::Resize(VulkanWindow& window) {
    m_width = window.swapchainExtent.width;
    m_height = window.swapchainExtent.height;

    // for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++) {
    //     VkDescriptorImageInfo imageInfo = {};
    //     imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //     imageInfo.imageView = m_LightingPass.imageView;  // need the g-buffer normals texture image view
    //     imageInfo.sampler = Enigma::defaultSampler;
    //     UpdateDescriptorSet(context, 1, imageInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    // }
}
void UIPass::CreateRenderPass() {
    VkAttachmentDescription attachment{};
    attachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;                // load the contents of the previous
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;             // store the result
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    // not using stencil, use don't care
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // not using stencil, use don't care
    attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;    // layout of the resource as it enters render pass
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // layout of resource at the end of the render pass

    VkAttachmentDescription attachments[1] = {attachment};

    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;

    VkSubpassDependency dependency[1]{};
    // Wait for EXTERNAL render pass to finish outputting
    dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency[0].srcAccessMask = 0;
    dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // Destination needs to wait for EXTERNAL to finishing outputting before writing
    dependency[0].dstSubpass = 0;
    dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkRenderPassCreateInfo rp_info{};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.attachmentCount = 1;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 1;
    rp_info.pDependencies = dependency;

    ENIGMA_VK_CHECK(vkCreateRenderPass(context.device, &rp_info, nullptr, &m_RenderPass),
                    "Failed to create imgui Render Pass.");
}
void UIPass::CreateBloodPipeline() {
    ShaderModule vertexShader = CreateShaderModule(VERTEX_BLOOD, context.device);
    ShaderModule fragmentShader = CreateShaderModule(FRAGMENT_BLOOD, context.device);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexShader.handle;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentShader.handle;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription{0, sizeof(float) * 2, VK_VERTEX_INPUT_RATE_VERTEX};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)window.swapchainExtent.width;
    viewport.height = (float)window.swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = window.swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterInfo{};
    rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.depthClampEnable = VK_FALSE;
    rasterInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterInfo.depthBiasClamp = VK_FALSE;
    rasterInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo samplingInfo{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendStates[1]{};
    blendStates[0].blendEnable = VK_FALSE;
    blendStates[0].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendInfo{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = blendStates;

    VkPipelineDepthStencilStateCreateInfo depthInfo{};
    depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthTestEnable = VK_FALSE;
    depthInfo.depthWriteEnable = VK_FALSE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthInfo.minDepthBounds = 0.0f;
    depthInfo.maxDepthBounds = 1.0f;

    // Push constant
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstant;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult res = vkCreatePipelineLayout(context.device, &layoutInfo, nullptr, &layout);

    ENIGMA_VK_CHECK(res, "Failed to create pipeline layout");

    m_bloodPipelineLayout = PipelineLayout(context.device, layout);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterInfo;
    pipelineInfo.pMultisampleState = &samplingInfo;
    pipelineInfo.pDepthStencilState = &depthInfo;
    pipelineInfo.pColorBlendState = &blendInfo;
    pipelineInfo.pDynamicState = 0;
    pipelineInfo.layout = m_bloodPipelineLayout.handle;
    pipelineInfo.renderPass = window.renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    ENIGMA_VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                    "Failed to create graphics pipeline.");

    m_bloodPipeline = Pipeline(context.device, pipeline);

    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    pipeline = VK_NULL_HANDLE;
    ENIGMA_VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                    "Failed to create graphics pipeline.");

    m_bloodPipeline2 = Pipeline(context.device, pipeline);
}
void UIPass::CreateBloodVertexBuffer() {
    bloodVertices = {
        {0, 0},
        {1, 0},
        {1, -1},
        {0, -1},
    };
    auto vertexSize = bloodVertices.size() * sizeof(glm::vec2);
    bloodVertexBuffer = CreateBuffer(
        context.allocator, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    void* data = nullptr;
    ENIGMA_VK_CHECK(vmaMapMemory(context.allocator.allocator, bloodVertexBuffer.allocation, &data),
                    "Failed to map staging buffer memory while loading model.");
    std::memcpy(data, bloodVertices.data(), vertexSize);
    vmaUnmapMemory(context.allocator.allocator, bloodVertexBuffer.allocation);
}
void UIPass::CreateHeadPipeline() {
    ShaderModule vertexShader = CreateShaderModule(VERTEX_HEAD, context.device);
    ShaderModule fragmentShader = CreateShaderModule(FRAGMENT_HEAD, context.device);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexShader.handle;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentShader.handle;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription{0, sizeof(float) * 2, VK_VERTEX_INPUT_RATE_VERTEX};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)window.swapchainExtent.width;
    viewport.height = (float)window.swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = window.swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterInfo{};
    rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.depthClampEnable = VK_FALSE;
    rasterInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterInfo.depthBiasClamp = VK_FALSE;
    rasterInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo samplingInfo{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendStates[1]{};
    blendStates[0].blendEnable = VK_FALSE;
    blendStates[0].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendInfo{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = blendStates;

    VkPipelineDepthStencilStateCreateInfo depthInfo{};
    depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthTestEnable = VK_FALSE;
    depthInfo.depthWriteEnable = VK_FALSE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthInfo.minDepthBounds = 0.0f;
    depthInfo.maxDepthBounds = 1.0f;

    // Push constant
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_headDescriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstant;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkResult res = vkCreatePipelineLayout(context.device, &layoutInfo, nullptr, &layout);

    ENIGMA_VK_CHECK(res, "Failed to create pipeline layout");

    m_headPipelineLayout = PipelineLayout(context.device, layout);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterInfo;
    pipelineInfo.pMultisampleState = &samplingInfo;
    pipelineInfo.pDepthStencilState = &depthInfo;
    pipelineInfo.pColorBlendState = &blendInfo;
    pipelineInfo.pDynamicState = 0;
    pipelineInfo.layout = m_headPipelineLayout.handle;
    pipelineInfo.renderPass = window.renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    ENIGMA_VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                    "Failed to create graphics pipeline.");

    m_headPipeline = Pipeline(context.device, pipeline);
}
void UIPass::CreateHeadImage() {
    auto path = "../resources/textures/head.jpg";
    m_headImage = Enigma::CreateTexture(context, path);

    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)};
    m_headDescriptorSetLayout = CreateDescriptorSetLayout(context, bindings);

    std::vector<VkDescriptorSet> descriptorSets;
    Enigma::AllocateDescriptorSets(context, Enigma::descriptorPool, m_headDescriptorSetLayout, 1, descriptorSets);
    m_headDescriptorSet = descriptorSets[0];

    VkDescriptorImageInfo imageinfo{Enigma::defaultSampler, m_headImage.imageView,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_headDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageinfo;

    vkUpdateDescriptorSets(context.device, 1, &descriptorWrite, 0, nullptr);
}
void UIPass::CreateBlood() {
    CreateBloodPipeline();
    CreateBloodVertexBuffer();

    m_headTransformMatrix = glm::mat4(1);
    float xscale = 0.1;
    float yscale = xscale * float(window.swapchainExtent.width) / float(window.swapchainExtent.height);
    m_headTransformMatrix = glm::mat4(1);
    m_headTransformMatrix[0][0] = xscale;
    m_headTransformMatrix[1][1] = yscale;
    m_headTransformMatrix[3][0] = -0.9;
    m_headTransformMatrix[3][1] = 0.9;

    CreateHeadImage();
    CreateHeadPipeline();
}
void UIPass::UpdateBlood(float value) {
    //
    float xscale = 0.2;
    float yscale = 0.05;
    bloodTransformMatrix = glm::mat4(1);
    bloodTransformMatrix[0][0] = value * xscale;
    bloodTransformMatrix[1][1] = yscale;
    bloodTransformMatrix[3][0] = -0.79;
    bloodTransformMatrix[3][1] = 0.9;
}
void UIPass::DrawBlood(VkCommandBuffer cmdBuffer) {
    VkDeviceSize offset[] = {0};
    VkBuffer buffers[] = {bloodVertexBuffer.buffer};
    vkDeviceWaitIdle(context.device);
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, buffers, offset);

    // set blood value
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloodPipeline.handle);
    // UpdateBlood(Enigma::WorldInst.player->health / 100.f);
    UpdateBlood(0.5f);
    vkCmdPushConstants(cmdBuffer, m_bloodPipelineLayout.handle, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                       &bloodTransformMatrix);
    vkCmdDraw(cmdBuffer, 4, 1, 0, 0);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloodPipeline2.handle);
    // do not change this
    UpdateBlood(1);
    vkCmdPushConstants(cmdBuffer, m_bloodPipelineLayout.handle, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                       &bloodTransformMatrix);
    vkCmdDraw(cmdBuffer, 4, 1, 0, 0);

    // draw head
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_headPipeline.handle);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_headPipelineLayout.handle, 0, 1,
                            &m_headDescriptorSet, 0, 0);
    vkCmdPushConstants(cmdBuffer, m_bloodPipelineLayout.handle, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                       &m_headTransformMatrix);
    vkCmdDraw(cmdBuffer, 4, 1, 0, 0);
}
}  // namespace Enigma