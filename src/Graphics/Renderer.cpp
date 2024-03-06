#include "Renderer.h"
#include <fstream>

namespace Enigma
{
	Renderer::Renderer(const VulkanContext& context, VulkanWindow& window, Camera* camera, Allocator& allocator) : context{ context }, window{ window }, camera{ camera }, allocator{ allocator } {
	
		// This will set up :
		// Fences
		// Image available and render finished semaphores
		// Command pool and command buffers;

		/*allocator = Enigma::MakeAllocator(context);*/

		m_sceneUBO.resize(Enigma::MAX_FRAMES_IN_FLIGHT);

		for(auto& buffer : m_sceneUBO)
			buffer = Enigma::CreateBuffer(allocator, sizeof(CameraTransform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
		
		CreateRendererResources();
		CreateGraphicsPipeline();

		m_World.Meshes.push_back(new Model("C:/Users/Billy/Documents/Enigma/resources/sponza_with_ship.obj", allocator, context));
		m_World.Meshes.push_back(new Model("C:/Users/Billy/Documents/Enigma/resources/cube.obj", allocator, context));
		m_World.Meshes.push_back(new Model("C:/Users/Billy/Documents/Enigma/resources/sponza_with_ship.obj", allocator, context));
		m_World.Meshes.push_back(new Player("C:/Users/Billy/Documents/Enigma/resources/sponza_with_ship.obj", allocator, context));
		m_World.Meshes.at(0)->translation = glm::vec3(100.f, 0.f, 100.f);
		m_World.Meshes.at(1)->scale = glm::vec3(1000.f, 0.01f, 1000.f);
		m_World.Meshes.at(2)->translation = glm::vec3(-100.f, 0.f, -100.f);
		m_World.Meshes.at(2)->rotationX = 90.f;
	}

	Renderer::~Renderer()
	{
		// Ensure all commands have finished and the GPU is now idle
		vkDeviceWaitIdle(context.device);

		for (const auto& model : m_World.Meshes)
		{
			delete model;
		}

		// destroy the command buffers
		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkFreeCommandBuffers(context.device, m_renderCommandPools[i].handle, 1, &m_renderCommandBuffers[i]);
		}
	}

	void Renderer::CreateRendererResources()
	{

		m_sceneDescriptorSets.reserve(Enigma::MAX_FRAMES_IN_FLIGHT);

		{
			std::vector<VkDescriptorSetLayoutBinding> bindings = {
				CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			};
			Enigma::descriptorLayout = CreateDescriptorSetLayout(context, bindings);
		}

		{
			std::vector<VkDescriptorSetLayoutBinding> bindings = {
				CreateDescriptorBinding(0, 40, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			Enigma::descriptorLayoutModel = CreateDescriptorSetLayout(context, bindings);
		}

		VkDescriptorPoolSize bufferPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
		bufferPoolSize.descriptorCount = 512;
		VkDescriptorPoolSize samplerPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
		samplerPoolSize.descriptorCount = 512;

		std::vector<VkDescriptorPoolSize> poolSize = { bufferPoolSize, samplerPoolSize };

		VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		info.poolSizeCount = static_cast<uint32_t>(poolSize.size());
		info.pPoolSizes = poolSize.data();
		info.maxSets = 1024;

		ENIGMA_VK_ERROR(vkCreateDescriptorPool(context.device, &info, nullptr, &Enigma::descriptorPool), "Failed to create descriptor pool in renderer");

		AllocateDescriptorSets(context, Enigma::descriptorPool, Enigma::descriptorLayout, Enigma::MAX_FRAMES_IN_FLIGHT, m_sceneDescriptorSets);

		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_sceneUBO[i].buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(CameraTransform);
			UpdateDescriptorSet(context, 0, bufferInfo, m_sceneDescriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		}

		Enigma::sampler = Enigma::CreateSampler(context);

		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
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

		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
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

		vkWaitForFences(context.device, 1, &m_fences[Enigma::currentFrame].handle, VK_TRUE, UINT64_MAX);
		vkResetFences(context.device, 1, &m_fences[Enigma::currentFrame].handle);
		
		// index of next available image to render to 
		//
		uint32_t index;
		VkResult getImageIndex = vkAcquireNextImageKHR(context.device, window.swapchain, UINT64_MAX, m_imagAvailableSemaphores[currentFrame].handle, VK_NULL_HANDLE, &index);

		void* data = nullptr;
		vmaMapMemory(allocator.allocator, m_sceneUBO[Enigma::currentFrame].allocation, &data);
		std::memcpy(data, &camera->GetCameraTransform(), sizeof(camera->GetCameraTransform()));
		vmaUnmapMemory(allocator.allocator, m_sceneUBO[Enigma::currentFrame].allocation);

		vkResetCommandBuffer(m_renderCommandBuffers[Enigma::currentFrame], 0);

		// Rendering ( Record commands for submission )
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VK_CHECK(vkBeginCommandBuffer(m_renderCommandBuffers[Enigma::currentFrame], &beginInfo));

			// begin recording commands 
			VkRenderPassBeginInfo renderpassInfo{};
			renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderpassInfo.renderPass = window.renderPass;
			renderpassInfo.framebuffer = window.swapchainFramebuffers[index];
			renderpassInfo.renderArea.extent = window.swapchainExtent;
			renderpassInfo.renderArea.offset = { 0,0 };

			VkClearValue clearColor[2];
			clearColor[0] = { 1.0f, 0.0f, 0.0f, 1.0f };
			if (window.hasResized)
			{
				clearColor[0] = { 0.0f, 0.0f, 1.0f, 1.0f };
			}
			clearColor[1].depthStencil.depth = 1.0f;
			renderpassInfo.clearValueCount = 2;
			renderpassInfo.pClearValues = clearColor;

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)window.swapchainExtent.width;
			viewport.height = (float)window.swapchainExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(m_renderCommandBuffers[Enigma::currentFrame], 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0,0 };
			scissor.extent = window.swapchainExtent;
			vkCmdSetScissor(m_renderCommandBuffers[Enigma::currentFrame], 0, 1, &scissor);

			vkCmdBeginRenderPass(m_renderCommandBuffers[Enigma::currentFrame], &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(m_renderCommandBuffers[Enigma::currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle);
			vkCmdBindDescriptorSets(m_renderCommandBuffers[Enigma::currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout.handle, 0, 1, &m_sceneDescriptorSets[Enigma::currentFrame], 0, nullptr);
			
			for (const auto& model : m_World.Meshes)
			{
				model->Draw(m_renderCommandBuffers[Enigma::currentFrame], m_pipelineLayout.handle);
			}
			
			vkCmdEndRenderPass(m_renderCommandBuffers[Enigma::currentFrame]);
			vkEndCommandBuffer(m_renderCommandBuffers[Enigma::currentFrame]);
		}

		VkPipelineStageFlags waitStage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		// Submit the commands
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_imagAvailableSemaphores[Enigma::currentFrame].handle;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_renderCommandBuffers[Enigma::currentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[Enigma::currentFrame].handle;

		VK_CHECK(vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, m_fences[Enigma::currentFrame].handle));

		VkPresentInfoKHR present{};
		present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present.swapchainCount = 1;
		present.pSwapchains = &window.swapchain;
		present.pImageIndices = &index;
		present.waitSemaphoreCount = 1;
		present.pWaitSemaphores = &m_renderFinishedSemaphores[Enigma::currentFrame].handle;


		VkResult res = vkQueuePresentKHR(context.graphicsQueue, &present);
		
		// Check if the swapchain is outdated
		// if it is, recreate the swapchain to ensure it's rendering at the new window size
		if (window.isSwapchainOutdated(res))
		{
			vkDeviceWaitIdle(context.device);
			Enigma::RecreateSwapchain(context, window); 
			CreateGraphicsPipeline();
			window.hasResized = true;
		}

		Enigma::currentFrame = (Enigma::currentFrame + 1) % Enigma::MAX_FRAMES_IN_FLIGHT;
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
		rasterInfo.cullMode = VK_CULL_MODE_NONE;
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

		VkPipelineDepthStencilStateCreateInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthInfo.depthTestEnable = VK_TRUE;
		depthInfo.depthWriteEnable = VK_TRUE;;
		depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthInfo.minDepthBounds = 0.0f;
		depthInfo.maxDepthBounds = 1.0f;

		// Make the pipeline layout here ( REMOVE THIS eventually )
		std::vector<VkDescriptorSetLayout> l = { Enigma::descriptorLayout, Enigma::descriptorLayoutModel};
		
		// Push constant
		VkPushConstantRange pushConstant{};
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(Enigma::ModelPushConstant);

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (uint32_t)l.size();
		layoutInfo.pSetLayouts = l.data();
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstant;

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
		pipelineInfo.pDepthStencilState = &depthInfo;
		pipelineInfo.pColorBlendState = &blendInfo;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = m_pipelineLayout.handle;
		pipelineInfo.renderPass = window.renderPass;
		pipelineInfo.subpass = 0;

		VkPipeline pipeline = VK_NULL_HANDLE;
		ENIGMA_VK_ERROR(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create graphics pipeline.");

		m_pipeline = Pipeline(context.device, pipeline);
	}
}
