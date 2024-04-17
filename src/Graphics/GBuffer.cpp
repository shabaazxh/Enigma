#include "GBuffer.h"

namespace Enigma
{
	GBuffer::GBuffer(const VulkanContext& context, const VulkanWindow& window, GBufferTargets& targets) : context{context}
	{
		m_width = window.swapchainExtent.width;
		m_height = window.swapchainExtent.height;

		m_sceneUBO.resize(Enigma::MAX_FRAMES_IN_FLIGHT);

		for (auto& buffer : m_sceneUBO)
			buffer = Enigma::CreateBuffer(context.allocator, sizeof(CameraTransform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		targets.normals = Enigma::CreateImageTexture2D(
			context,
			m_width,
			m_height,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, 1
		);

		targets.albedo = Enigma::CreateImageTexture2D(
			context,
			m_width,
			m_height,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, 1
		);

		targets.depth = Enigma::CreateImageTexture2D(
			context,
			m_width,
			m_height,
			VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT, 1
		);

		BuildDescriptorSetLayout(context);
		CreateRenderPass(context.device);
		CreateFramebuffer(context.device, targets);
		CreatePipeline(context.device, window.swapchainExtent);

	}

	GBuffer::~GBuffer()
	{
		if (m_framebuffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(context.device, m_framebuffer, nullptr);
		}

		if (m_RenderPass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(context.device, m_RenderPass, nullptr);
		}

		if (m_descriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(context.device, m_descriptorSetLayout, nullptr);
		}
	}
	// only should be outputting normals ( can reconstuct position from depth )
	void GBuffer::Execute(VkCommandBuffer cmd, const std::vector<Model*>& models)
	{
		VkRenderPassBeginInfo rpBegin{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		rpBegin.renderPass = m_RenderPass;
		rpBegin.framebuffer = m_framebuffer;
		rpBegin.renderArea.extent = { m_width, m_height };
		rpBegin.renderArea.offset = { 0,0 };

		VkClearValue clearValues[3]; // 3 color, 1 depth
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil.depth = 1.0f;
		clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		rpBegin.clearValueCount = 3;
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
		scissor.offset = { 0,0 };
		scissor.extent = { m_width, m_height };
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout.handle, 0, 1, &m_sceneDescriptorSets[Enigma::currentFrame], 0, nullptr);

		for (const auto& model : models)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle);
			model->Draw(cmd, m_pipelineLayout.handle);
		}

		vkCmdEndRenderPass(cmd);
	}

	void GBuffer::Update(Camera* camera)
	{
		void* data = nullptr;
		vmaMapMemory(context.allocator.allocator, m_sceneUBO[Enigma::currentFrame].allocation, &data);
		std::memcpy(data, &camera->GetCameraTransform(), sizeof(camera->GetCameraTransform()));
		vmaUnmapMemory(context.allocator.allocator, m_sceneUBO[Enigma::currentFrame].allocation);
	}

	void GBuffer::CreateFramebuffer(VkDevice device, GBufferTargets& targets)
	{
		std::vector<VkImageView> attachments = { targets.normals.imageView, targets.depth.imageView, targets.albedo.imageView};
		VkFramebufferCreateInfo fbInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbInfo.pAttachments = attachments.data();
		fbInfo.renderPass = m_RenderPass;
		fbInfo.width = m_width;
		fbInfo.height = m_height;
		fbInfo.layers = 1;

		ENIGMA_VK_CHECK(vkCreateFramebuffer(device, &fbInfo, nullptr, &m_framebuffer), "Failed to create G-Buffer framebuffer.");
	}

	void GBuffer::CreateRenderPass(VkDevice device)
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentDescription albedoColourAttachment = {};
		albedoColourAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		albedoColourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		albedoColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		albedoColourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		albedoColourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		albedoColourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		albedoColourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		albedoColourAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = VK_FORMAT_D32_SFLOAT;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentDescription attachments[3] = { colorAttachment, depthAttachment, albedoColourAttachment };

		//VkAttachmentReference colorReference =  { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		//VkAttachmentReference albedoReference = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference =  { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		std::vector<VkAttachmentReference> colorReference = { 
			{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, 
			{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL} 
		};

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = (uint32_t)colorReference.size();
		subpass.pColorAttachments = colorReference.data();
		subpass.pDepthStencilAttachment = &depthReference;

		VkSubpassDependency dependency[3] = {};
		// Wait for EXTERNAL render pass to finish outputting
		dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency[0].srcAccessMask = 0;
		dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		// This subpass is the destination, wait for EXTERNAL to finish outputting before writing to it 
		dependency[0].dstSubpass = 0;
		dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		// Depth dependency 
		// Source subpass is current subpass which writes to the depth image at late frag tests
		dependency[1].srcSubpass = 0;
		dependency[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		// destination subpass is external, uses depth in fragment shader 
		dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependency[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependency[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		dependency[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependency[2].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency[2].srcAccessMask = 0;
		dependency[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;   
		dependency[2].dstSubpass = 0;
		dependency[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassInfo.attachmentCount = static_cast<uint32_t>(3);
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 3;
		renderPassInfo.pDependencies = dependency;

		ENIGMA_VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass), "Failed to create G-Buffer render pass");
	}
	void GBuffer::CreatePipeline(VkDevice device, VkExtent2D swapchainExtent)
	{
		ShaderModule vertexShader = CreateShaderModule(VERTEX, device);
		ShaderModule fragmentShader = CreateShaderModule(FRAGMENT, device);

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

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Enigma::Vertex::GetBindingDescription();
		auto attributeDescriptions = Enigma::Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		//VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_width;
		viewport.height = (float)m_height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = swapchainExtent;

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

		VkPipelineMultisampleStateCreateInfo samplingInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState blendStates[2]{};

		blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendStates[0].blendEnable = VK_TRUE;
		blendStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendStates[0].colorBlendOp = VK_BLEND_OP_ADD;
		blendStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendStates[0].alphaBlendOp = VK_BLEND_OP_ADD;

		blendStates[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendStates[1].blendEnable = VK_TRUE;
		blendStates[1].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendStates[1].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendStates[1].colorBlendOp = VK_BLEND_OP_ADD;
		blendStates[1].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendStates[1].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendStates[1].alphaBlendOp = VK_BLEND_OP_ADD;

		//blendStates[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		//blendStates[2].blendEnable = VK_TRUE;
		//blendStates[2].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		//blendStates[2].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		//blendStates[2].colorBlendOp = VK_BLEND_OP_ADD;
		//blendStates[2].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		//blendStates[2].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		//blendStates[2].alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo blendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		blendInfo.logicOpEnable = VK_FALSE;
		blendInfo.attachmentCount = 2;
		blendInfo.pAttachments = blendStates;

		VkPipelineDepthStencilStateCreateInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthInfo.depthTestEnable = VK_TRUE;
		depthInfo.depthWriteEnable = VK_TRUE;
		depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthInfo.minDepthBounds = 0.0f;
		depthInfo.maxDepthBounds = 1.0f;

		// Push constant
		VkPushConstantRange pushConstant{};
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(Enigma::ModelPushConstant);

		std::vector<VkDescriptorSetLayout> layouts = { m_descriptorSetLayout, Enigma::descriptorLayoutModel };

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (uint32_t)layouts.size();
		layoutInfo.pSetLayouts = layouts.data();
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstant;

		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkResult res = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout);

		ENIGMA_VK_CHECK(res, "Failed to create pipeline layout");

		m_pipelineLayout = PipelineLayout(device, layout);

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
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = m_pipelineLayout.handle;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;

		VkPipeline pipeline = VK_NULL_HANDLE;
		ENIGMA_VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create graphics pipeline.");

		m_pipeline = Pipeline(device, pipeline);
	}
	void GBuffer::BuildDescriptorSetLayout(const VulkanContext& context)
	{
		m_sceneDescriptorSets.reserve(Enigma::MAX_FRAMES_IN_FLIGHT);

		// Scene descriptor set layout 
		// Set = 0
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings = {
				CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			m_descriptorSetLayout = CreateDescriptorSetLayout(context, bindings);
		}

		// Set = 1
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings = {
				CreateDescriptorBinding(0, 40, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			Enigma::descriptorLayoutModel = CreateDescriptorSetLayout(context, bindings);
		}

		AllocateDescriptorSets(context, Enigma::descriptorPool, m_descriptorSetLayout, Enigma::MAX_FRAMES_IN_FLIGHT, m_sceneDescriptorSets);

		// Binding 0
		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_sceneUBO[i].buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(CameraTransform);
			UpdateDescriptorSet(context, 0, bufferInfo, m_sceneDescriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		}
	}
}

