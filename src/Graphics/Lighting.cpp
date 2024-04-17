#include "Lighting.h"
#include "../Core/World.h"

namespace Enigma
{
	Lighting::Lighting(const VulkanContext& context, const VulkanWindow& window, GBufferTargets& targets) : context{ context }
	{
		m_width = window.swapchainExtent.width;
		m_height = window.swapchainExtent.height;

		m_uniformBO.resize(Enigma::MAX_FRAMES_IN_FLIGHT);
		m_lightingUBO.resize(Enigma::MAX_FRAMES_IN_FLIGHT);

		for (auto& buffer : m_uniformBO)
			buffer = Enigma::CreateBuffer(context.allocator, sizeof(CameraTransform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		for(auto& buffer : m_lightingUBO)
			buffer = Enigma::CreateBuffer(context.allocator, sizeof(CameraTransform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
		
		renderTarget = Enigma::CreateImageTexture2D(
			context,
			m_width,
			m_height,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, 1
		);

		CreateRenderPass(context.device);
		CreateFramebuffer(context.device);
		BuildDescriptorSetLayout(context, targets);
		CreatePipeline(context.device, window.swapchainExtent);
	}

	Lighting::~Lighting()
	{
		// destroy vulkan allocated resources 
	}

	void Lighting::Execute(VkCommandBuffer cmd)
	{
		VkRenderPassBeginInfo rpBegin{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		rpBegin.renderPass = m_RenderPass;
		rpBegin.framebuffer = m_framebuffer;
		rpBegin.renderArea.extent = { m_width, m_height };
		rpBegin.renderArea.offset = { 0,0 };

		VkClearValue clearValues[2]; // 3 color, 1 depth
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil.depth = 1.0f;
		rpBegin.clearValueCount = 2;
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
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout.handle, 0, 1, &m_descriptorSets[Enigma::currentFrame], 0, nullptr);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle);
		vkCmdDraw(cmd, 3, 1, 0, 0);

		vkCmdEndRenderPass(cmd);
	}

	void Lighting::Update(Camera* camera)
	{
		void* data = nullptr;
		vmaMapMemory(context.allocator.allocator, m_uniformBO[Enigma::currentFrame].allocation, &data);
		std::memcpy(data, &camera->GetCameraTransform(), sizeof(camera->GetCameraTransform()));
		vmaUnmapMemory(context.allocator.allocator, m_uniformBO[Enigma::currentFrame].allocation);

		// if we have a directional light defined by the user then use it 
		if (!Enigma::WorldInst.Lights.empty())
		{
			// Get the light
			Light DirLightSource = Enigma::WorldInst.Lights[0]; // since the arr is not empty, index 0 is guaranteed to exist
			m_lightUBO = {};
			m_lightUBO.lightPosition = DirLightSource.m_position;
			m_lightUBO.lightDirection = DirLightSource.m_direction;
			m_lightUBO.lightColour = DirLightSource.m_color;
		}
		else
		{	// if we have no user-defined directional light, provide a default one 
			m_lightUBO = {};
			m_lightUBO.lightPosition = glm::vec4(0.0f, 10.0f, 0.0f, 1.0f);
			m_lightUBO.lightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 1.0);
			m_lightUBO.lightColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
		
		// Update the lighting uniform
		vmaMapMemory(context.allocator.allocator, m_lightingUBO[Enigma::currentFrame].allocation, &data);
		std::memcpy(data, &m_lightUBO, sizeof(m_lightUBO));
		vmaUnmapMemory(context.allocator.allocator, m_lightingUBO[Enigma::currentFrame].allocation);
	}

	void Lighting::CreateFramebuffer(VkDevice device)
	{
		std::vector<VkImageView> attachments = { renderTarget.imageView };
		VkFramebufferCreateInfo fbInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbInfo.pAttachments = attachments.data();
		fbInfo.renderPass = m_RenderPass;
		fbInfo.width = m_width;
		fbInfo.height = m_height;
		fbInfo.layers = 1;

		ENIGMA_VK_CHECK(vkCreateFramebuffer(device, &fbInfo, nullptr, &m_framebuffer), "Failed to create lighting pass frame buffer");
	}

	void Lighting::CreateRenderPass(VkDevice device)
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

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;

		VkSubpassDependency dependency[1] = {};
		dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL; // this is the render pass before this one
		dependency[0].dstSubpass = 0; // this is the current lighting render pass 
		dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // this says that the lighing pass in the fragment shader relies on the color attachment output stage of the previous pass
		dependency[0].srcAccessMask = VK_ACCESS_NONE; // dependency[0].srcAccessMask = 0;
		dependency[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // // this says that the lighing pass in the fragment shader relies on the color attachment output stage of the previous pass
		dependency[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkAttachmentDescription attachments[1] = { colorAttachment };

		VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassInfo.attachmentCount = static_cast<uint32_t>(1);
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = dependency;


		ENIGMA_VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass), "Failed to create lighting pass render pass.");

	}

	void Lighting::CreatePipeline(VkDevice device, VkExtent2D swapchainExtent)
	{
		ShaderModule vertexShader = CreateShaderModule(LIGHTING_VERTEX, device);
		ShaderModule fragmentShader = CreateShaderModule(LIGHTING_FRAGMENT, device);

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

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

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
		rasterInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterInfo.depthBiasClamp = VK_FALSE;
		rasterInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo samplingInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState blendStates[1]{};

		blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendStates[0].blendEnable = VK_TRUE;
		blendStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendStates[0].colorBlendOp = VK_BLEND_OP_ADD;
		blendStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendStates[0].alphaBlendOp = VK_BLEND_OP_ADD;


		VkPipelineColorBlendStateCreateInfo blendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
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

		std::vector<VkDescriptorSetLayout> layouts = { m_descriptorSetLayout };

		// no descriptor set layouts currently since it's not needed
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = 1;
		layoutInfo.pSetLayouts = layouts.data();

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

	void Lighting::BuildDescriptorSetLayout(const VulkanContext& context, GBufferTargets& gBufferTargets)
	{
		m_descriptorSets.reserve(Enigma::MAX_FRAMES_IN_FLIGHT);

		// Scene descriptor set layout 
		// Set = 0
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings = {
				CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
				CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
				CreateDescriptorBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
				CreateDescriptorBinding(3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
				CreateDescriptorBinding(4, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			m_descriptorSetLayout = CreateDescriptorSetLayout(context, bindings);
		}

		AllocateDescriptorSets(context, Enigma::descriptorPool, m_descriptorSetLayout, Enigma::MAX_FRAMES_IN_FLIGHT, m_descriptorSets);

		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_uniformBO[i].buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(CameraTransform);
			UpdateDescriptorSet(context, 0, bufferInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		}
		
		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = gBufferTargets.normals.imageView;// need the g-buffer normals texture image view
			imageInfo.sampler = Enigma::defaultSampler;

			UpdateDescriptorSet(context, 1, imageInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			imageInfo.imageView = gBufferTargets.depth.imageView;
			imageInfo.sampler = Enigma::defaultSampler;

			UpdateDescriptorSet(context, 2, imageInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			imageInfo.imageView = gBufferTargets.albedo.imageView;
			imageInfo.sampler = Enigma::defaultSampler;

			UpdateDescriptorSet(context, 3, imageInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_lightingUBO[i].buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(LightUBO);
			UpdateDescriptorSet(context, 4, bufferInfo, m_descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		}
	}

};