#include "Renderer.h"
#include <fstream>

namespace
{
	Enigma::Semaphore CreateSemaphore(VkDevice device);
	Enigma::CommandPool CreateCommandPool(VkDevice device, VkCommandPoolCreateFlagBits flags, uint32_t queueFamilyIndex);
	Enigma::ShaderModule CreateShaderModule(const std::string& filename, VkDevice device);
	VkDescriptorSet alloc_desc_set(const Enigma::VulkanContext& aContext, VkDescriptorPool aPool, VkDescriptorSetLayout aSetLayout);
	Enigma::DescriptorPool create_descriptor_pool(const Enigma::VulkanContext& aContext, std::uint32_t aMaxDescriptors = 2048, std::uint32_t aMaxSets = 1024);
	Enigma::DescriptorSetLayout create_scene_descriptor_layout(Enigma::VulkanContext const& aWindow);
	void update_descriptor_sets(Enigma::VulkanContext const& aWindow, VkBuffer sceneUBO, VkDescriptorSet sceneDescriptors);
}

namespace Enigma
{
	Renderer::Renderer(const VulkanContext& context, VulkanWindow& window) : context{ context }, window{ window } {
	
		// This will set up :
		// Fences
		// Image available and render finished semaphores
		// Command pool and command buffers;

		allocator = Enigma::MakeAllocator(context);
		CreateRendererResources();
		CreateGraphicsPipeline();

		sceneLayout = create_scene_descriptor_layout(context);

		sceneUBO = CreateBuffer(allocator, sizeof(glsl::SceneUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
		dpool = create_descriptor_pool(context);
		sceneDescriptors = alloc_desc_set(context, dpool.handle, sceneLayout.handle);
		update_descriptor_sets(context, sceneUBO.buffer, sceneDescriptors);

		update_scene_uniforms(sceneUniform, window.swapchainExtent.width, window.swapchainExtent.height, state);

		m_renderables.push_back(new Model("C:/Users/Billy/Documents/Enigma/resources/cube.obj", "C:/Users/Billy/Documents/Enigma/resources/bricks.png", allocator, context, dpool.handle));
	}

	Renderer::~Renderer()
	{
		// Ensure all commands have finished and the GPU is now idle
		vkDeviceWaitIdle(context.device);

		for (const auto& model : m_renderables)
		{
			delete model;
		}

		// destroy the command buffers
		for (size_t i = 0; i < max_frames_in_flight; i++)
		{
			vkFreeCommandBuffers(context.device, m_renderCommandPools[i].handle, 1, &m_renderCommandBuffers[i]);
		}
	}

	void Renderer::CreateRendererResources()
	{
		for (size_t i = 0; i < max_frames_in_flight; i++)
		{
			VkFenceCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VkFence fence = VK_NULL_HANDLE;
			VK_CHECK(vkCreateFence(context.device, &info, nullptr, &fence));

			Fence f{ context.device, fence };
			m_fences.push_back(std::move(f));


			// Create semaphores

			// Image avaialble semaphore
			Semaphore imgAvaialbleSemaphore = CreateSemaphore(context.device);
			m_imagAvailableSemaphores.push_back(std::move(imgAvaialbleSemaphore));

			// Render finished semaphore
			Semaphore renderFinishedSemaphore = CreateSemaphore(context.device);
			m_renderFinishedSemaphores.push_back(std::move(renderFinishedSemaphore));
		}

		// Command pool set up 

		for (size_t i = 0; i < max_frames_in_flight; i++)
		{
			Enigma::CommandPool commandPool = CreateCommandPool(context.device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, context.graphicsFamilyIndex);
			m_renderCommandPools.push_back(std::move(commandPool));

			// Allocate command buffers from command pool
			VkCommandBufferAllocateInfo cmdAlloc{};
			cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdAlloc.commandPool = m_renderCommandPools[i].handle;
			cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdAlloc.commandBufferCount = 1;

			VkCommandBuffer cmd = VK_NULL_HANDLE;
			VK_CHECK(vkAllocateCommandBuffers(context.device, &cmdAlloc, &cmd));
			m_renderCommandBuffers.push_back(cmd);
		}
	}

	void Renderer::DrawScene()
	{

		vkWaitForFences(context.device, 1, &m_fences[currentFrame].handle, VK_TRUE, UINT64_MAX);
		vkResetFences(context.device, 1, &m_fences[currentFrame].handle);
		
		// index of next available image to render to 
		//
		uint32_t index;
		VkResult getImageIndex = vkAcquireNextImageKHR(context.device, window.swapchain, UINT64_MAX, m_imagAvailableSemaphores[currentFrame].handle, VK_NULL_HANDLE, &index);

		update_scene_uniforms(sceneUniform, window.swapchainExtent.width, window.swapchainExtent.height, state);

		vkResetCommandBuffer(m_renderCommandBuffers[currentFrame], 0);

		// Rendering ( Record commands for submission )
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VK_CHECK(vkBeginCommandBuffer(m_renderCommandBuffers[currentFrame], &beginInfo));

			// begin recording commands 
			VkRenderPassBeginInfo renderpassInfo{};
			renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderpassInfo.renderPass = window.renderPass;
			renderpassInfo.framebuffer = window.swapchainFramebuffers[index];
			renderpassInfo.renderArea.extent = window.swapchainExtent;
			renderpassInfo.renderArea.offset = { 0,0 };

			VkClearValue clearColor = { 1.0f, 0.0f, 0.0f, 1.0f };
			if (window.hasResized)
			{
				clearColor = { 0.0f, 0.0f, 1.0f, 1.0f };
			}
			renderpassInfo.clearValueCount = 1;
			renderpassInfo.pClearValues = &clearColor;

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)window.swapchainExtent.width;
			viewport.height = (float)window.swapchainExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(m_renderCommandBuffers[currentFrame], 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0,0 };
			scissor.extent = window.swapchainExtent;
			vkCmdSetScissor(m_renderCommandBuffers[currentFrame], 0, 1, &scissor);

			vkCmdUpdateBuffer(m_renderCommandBuffers[currentFrame], sceneUBO.buffer, 0, sizeof(glsl::SceneUniform), &sceneUniform);

			vkCmdBeginRenderPass(m_renderCommandBuffers[currentFrame], &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(m_renderCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle);

			for (const auto& model : m_renderables)
			{
				model->Draw(m_renderCommandBuffers[currentFrame], sceneDescriptors, m_pipelineLayout.handle);
			}
			
			vkCmdEndRenderPass(m_renderCommandBuffers[currentFrame]);
			vkEndCommandBuffer(m_renderCommandBuffers[currentFrame]);
		}

		VkPipelineStageFlags waitStage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// Submit the commands
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_imagAvailableSemaphores[currentFrame].handle;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_renderCommandBuffers[currentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[currentFrame].handle;

		VK_CHECK(vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, m_fences[currentFrame].handle));

		VkPresentInfoKHR present{};
		present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present.swapchainCount = 1;
		present.pSwapchains = &window.swapchain;
		present.pImageIndices = &index;
		present.waitSemaphoreCount = 1;
		present.pWaitSemaphores = &m_renderFinishedSemaphores[currentFrame].handle;

		VkResult res = vkQueuePresentKHR(context.graphicsQueue, &present);
		
		// Check if the swapchain is outdated
		// if it is, recreate the swapchain to ensure it's rendering at the new window size
		if (window.isSwapchainOutdated(res))
		{
			vkDeviceWaitIdle(context.device);
			Enigma::RecreateSwapchain(context, window);
			window.hasResized = true;
		}

		currentFrame = (currentFrame + 1) % max_frames_in_flight;
	}

	void Renderer::CreateGraphicsPipeline()
	{
		ShaderModule vertexShader   = CreateShaderModule("C:/Users/Billy/Documents/Enigma/resources/Shaders/vertex.vert.spv", context.device);
		ShaderModule fragmentShader = CreateShaderModule("C:/Users/Billy/Documents/Enigma/resources/Shaders/fragment.frag.spv", context.device);

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

		auto bindingDescription    = Enigma::Vertex::GetBindingDescription();
		auto attributeDescriptions = Enigma::Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)window.swapchainExtent.width;
		viewport.height = (float)window.swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
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

		VkPipelineMultisampleStateCreateInfo samplingInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState blendStates[1]{};
		blendStates[0].blendEnable = VK_FALSE;
		blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo blendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		blendInfo.logicOpEnable = VK_FALSE;
		blendInfo.attachmentCount = 1;
		blendInfo.pAttachments = blendStates;


		// Make the pipeline layout here ( REMOVE THIS eventually )

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = 0;
		layoutInfo.pSetLayouts = nullptr;
		layoutInfo.pushConstantRangeCount = 0;
		layoutInfo.pPushConstantRanges = nullptr;

		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkResult res = vkCreatePipelineLayout(context.device, &layoutInfo, nullptr, &layout);
		
		ENIGMA_VK_ERROR(res, "Failed to create pipeline layout");

		m_pipelineLayout = PipelineLayout(context.device, layout);

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
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &blendInfo;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = m_pipelineLayout.handle;
		pipelineInfo.renderPass = window.renderPass;
		pipelineInfo.subpass = 0;

		VkPipeline pipeline = VK_NULL_HANDLE;
		ENIGMA_VK_ERROR(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create graphics pipeline.");

		m_pipeline = Pipeline(context.device, pipeline);
	}

	void Renderer::update_scene_uniforms(glsl::SceneUniform& aSceneUniforms, std::uint32_t aFramebufferWidth, std::uint32_t aFramebufferHeight, UserState const& aState)
	{
		float const aspect = aFramebufferWidth / float(aFramebufferHeight);
		aSceneUniforms.projection = glm::perspectiveRH_ZO((float)((60*M_PI)/180), aspect, 0.1f, 100.f);
		aSceneUniforms.projection[1][1] *= -1.f; // mirror Y axis

		aSceneUniforms.camera = glm::inverse(aState.camera2world);
		aSceneUniforms.projCam = aSceneUniforms.projection * aSceneUniforms.camera;
	}
}


namespace
{
	Enigma::Semaphore CreateSemaphore(VkDevice device)
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkSemaphore semaphore = VK_NULL_HANDLE;
		VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));

		return Enigma::Semaphore{ device, semaphore };
	}

	Enigma::CommandPool CreateCommandPool(VkDevice device, VkCommandPoolCreateFlagBits flags, uint32_t queueFamilyIndex)
	{
		VkCommandPoolCreateInfo cmdPool{};
		cmdPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPool.flags = flags;
		cmdPool.queueFamilyIndex = queueFamilyIndex;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VK_CHECK(vkCreateCommandPool(device, &cmdPool, nullptr, &commandPool));

		return Enigma::CommandPool{ device, commandPool };
	}

	Enigma::ShaderModule CreateShaderModule(const std::string& filename, VkDevice device)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open shader file.");
		}
		
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		VkShaderModuleCreateInfo shadermoduleInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		shadermoduleInfo.codeSize = buffer.size();
		shadermoduleInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		if (vkCreateShaderModule(device, &shadermoduleInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module");
		}

		return Enigma::ShaderModule(device, shaderModule);
	}

	VkDescriptorSet alloc_desc_set(const Enigma::VulkanContext& aContext, VkDescriptorPool aPool, VkDescriptorSetLayout aSetLayout) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = aPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &aSetLayout;
		VkDescriptorSet dset = VK_NULL_HANDLE;
		if (auto const res = vkAllocateDescriptorSets(aContext.device, &allocInfo, &dset); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed to create descriptor set");
		}

		return dset;
	}

	Enigma::DescriptorPool create_descriptor_pool(const Enigma::VulkanContext& aContext, std::uint32_t aMaxDescriptors, std::uint32_t aMaxSets) {
		VkDescriptorPoolSize const pools[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, aMaxDescriptors },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, aMaxDescriptors }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = aMaxSets;
		poolInfo.poolSizeCount = sizeof(pools) / sizeof(pools[0]);
		poolInfo.pPoolSizes = pools;

		VkDescriptorPool pool = VK_NULL_HANDLE;
		if (auto const res = vkCreateDescriptorPool(aContext.device, &poolInfo, nullptr, &pool); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed to create descriptor pool");
		}

		return Enigma::DescriptorPool(aContext.device, pool);
	}

	Enigma::DescriptorSetLayout create_scene_descriptor_layout(Enigma::VulkanContext const& aWindow) {
		VkDescriptorSetLayoutBinding bindings[1]{};
		bindings[0].binding = 0; // number must match the index of the corresponding
		// binding = N declaration in the shader(s)!
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
		layoutInfo.pBindings = bindings;

		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed to create descriptor set layout");
		}

		return Enigma::DescriptorSetLayout(aWindow.device, layout);
	}

	void update_descriptor_sets(Enigma::VulkanContext const& aWindow, VkBuffer sceneUBO, VkDescriptorSet sceneDescriptors) {
		VkWriteDescriptorSet desc[1]{};
		VkDescriptorBufferInfo sceneUboInfo{};
		sceneUboInfo.buffer = sceneUBO;
		sceneUboInfo.range = VK_WHOLE_SIZE;
		desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc[0].dstSet = sceneDescriptors;
		desc[0].dstBinding = 0;
		desc[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc[0].descriptorCount = 1;
		desc[0].pBufferInfo = &sceneUboInfo;

		constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
		vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);
	}
}