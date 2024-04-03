#include "Renderer.h"
#include <fstream>

namespace Enigma
{
	Renderer::Renderer(const VulkanContext& context, VulkanWindow& window, Camera* camera, World* world) : context{ context }, window{ window }, camera{ camera }, m_World{ world } {

		m_sceneUBO.resize(Enigma::MAX_FRAMES_IN_FLIGHT);

		for(auto& buffer : m_sceneUBO)
			buffer = Enigma::CreateBuffer(context.allocator, sizeof(CameraTransform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
		
		CreateRendererResources();

		m_pipeline = CreateGraphicsPipeline("../resources/Shaders/vertex.vert.spv", "../resources/Shaders/fragment.frag.spv", VK_FALSE, VK_TRUE, VK_TRUE, {Enigma::sceneDescriptorLayout, Enigma::descriptorLayoutModel}, m_pipelinePipelineLayout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		m_aabbPipeline = CreateGraphicsPipeline("../resources/Shaders/vertex.vert.spv", "../resources/Shaders/line.frag.spv", VK_FALSE, VK_TRUE, VK_TRUE, { Enigma::sceneDescriptorLayout, Enigma::descriptorLayoutModel }, m_pipelinePipelineLayout, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
		
		m_World = world;

		Model* temp = new Model("../resources/level1.obj", context, ENIGMA_LOAD_OBJ_FILE);
		m_World->Meshes.push_back(temp);
		Enemy* enemy1 = new Enemy("../resources/zombie-walk-test/source/Zombie_Walk1.fbx", context, ENIGMA_LOAD_FBX_FILE);
		m_World->Meshes.push_back(enemy1->model);
		enemy1->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
		enemy1->setTranslation(glm::vec3(60.f, 0.1f, 0.f));
		m_World->Enemies.push_back(enemy1);

		/*Enemy* enemy2 = new Enemy("../resources/zombie-walk-test/source/Zombie_Walk1.fbx", context, ENIGMA_LOAD_FBX_FILE);
		m_World->Meshes.push_back(enemy2->model);
		enemy2->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
		enemy2->setTranslation(glm::vec3(60.f, 0.1f, 50.f));
		m_World->Enemies.push_back(enemy2);*/

		//player = new Player(context);
		m_World->player = new Player("../resources/gun.obj", context, ENIGMA_LOAD_OBJ_FILE);
		if (!m_World->player->noModel) {
			m_World->Meshes.push_back(m_World->player->model);
		}
		m_World->player->setTranslation(glm::vec3(-100.f, 0.1f, -40.f));
		m_World->player->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
		m_World->player->setRotationY(180);
	}

	Renderer::~Renderer()
	{
		// Ensure all commands have finished and the GPU is now idle
		vkDeviceWaitIdle(context.device);

		for (auto& model : m_World->Meshes)
		{
			delete model;
		}

		// destroy the command buffers
		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkFreeCommandBuffers(context.device, m_renderCommandPools[i].handle, 1, &m_renderCommandBuffers[i]);
		}

		vkDestroySampler(context.device, Enigma::defaultSampler, nullptr);
		vkDestroyDescriptorSetLayout(context.device, Enigma::sceneDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(context.device, Enigma::descriptorLayoutModel, nullptr);
		vkDestroyDescriptorPool(context.device, Enigma::descriptorPool, nullptr);
	}

	void Renderer::CreateSceneDescriptorSetLayout()
	{
		m_sceneDescriptorSets.reserve(Enigma::MAX_FRAMES_IN_FLIGHT);

		// Scene descriptor set layout 
		// Set = 0
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings = {
				CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			Enigma::sceneDescriptorLayout = CreateDescriptorSetLayout(context, bindings);
		}

		// Set = 1
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings = {
				CreateDescriptorBinding(0, 40, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			Enigma::descriptorLayoutModel = CreateDescriptorSetLayout(context, bindings);
		}

		AllocateDescriptorSets(context, Enigma::descriptorPool, Enigma::sceneDescriptorLayout, Enigma::MAX_FRAMES_IN_FLIGHT, m_sceneDescriptorSets);

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

	void Renderer::CreateDescriptorSetLayouts()
	{
		VkDescriptorPoolSize bufferPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
		bufferPoolSize.descriptorCount = 512;
		VkDescriptorPoolSize samplerPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
		samplerPoolSize.descriptorCount = 512;
		VkDescriptorPoolSize storagePoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
		storagePoolSize.descriptorCount = 512;

		std::vector<VkDescriptorPoolSize> poolSize = { bufferPoolSize, samplerPoolSize, storagePoolSize };

		VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		info.poolSizeCount = static_cast<uint32_t>(poolSize.size());
		info.pPoolSizes = poolSize.data();
		info.maxSets = 1536;

		ENIGMA_VK_CHECK(vkCreateDescriptorPool(context.device, &info, nullptr, &Enigma::descriptorPool), "Failed to create descriptor pool in renderer");

		CreateSceneDescriptorSetLayout();
	}

	void Renderer::CreateRendererResources()
	{
		CreateDescriptorSetLayouts();

		Enigma::defaultSampler = Enigma::CreateSampler(context);

		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkFenceCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VkFence fence = VK_NULL_HANDLE;
			ENIGMA_VK_CHECK(vkCreateFence(context.device, &info, nullptr, &fence), "Failed to create Fence.");

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
			ENIGMA_VK_CHECK(vkAllocateCommandBuffers(context.device, &cmdAlloc, &cmd), "Failed to allocate command buffer");
			m_renderCommandBuffers.push_back(cmd);
		}
	}

	void Renderer::Update(Camera* cam)
	{
		void* data = nullptr;
		if (current_state != isPlayer) {
			if (isPlayer) {
				camera->SetPosition(m_World->player->translation + glm::vec3(0.f, 13.f, 0.f));
				camera->SetNearPlane(0.05f);
			}
			else {
				camera->SetNearPlane(1.f);
			}
			current_state = isPlayer;
		}
		vmaMapMemory(context.allocator.allocator, m_sceneUBO[Enigma::currentFrame].allocation, &data);
		std::memcpy(data, &camera->GetCameraTransform(), sizeof(camera->GetCameraTransform()));
		vmaUnmapMemory(context.allocator.allocator, m_sceneUBO[Enigma::currentFrame].allocation);
	}

	void Renderer::DrawScene()
	{

		vkWaitForFences(context.device, 1, &m_fences[Enigma::currentFrame].handle, VK_TRUE, UINT64_MAX);
		vkResetFences(context.device, 1, &m_fences[Enigma::currentFrame].handle);
		
		// index of next available image to render to 
		//
		uint32_t index;
		VkResult getImageIndex = vkAcquireNextImageKHR(context.device, window.swapchain, UINT64_MAX, m_imagAvailableSemaphores[currentFrame].handle, VK_NULL_HANDLE, &index);


		vkResetCommandBuffer(m_renderCommandBuffers[Enigma::currentFrame], 0);

		VkCommandBuffer cmd = m_renderCommandBuffers[Enigma::currentFrame];
		// Rendering ( Record commands for submission )
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			ENIGMA_VK_CHECK(vkBeginCommandBuffer(m_renderCommandBuffers[Enigma::currentFrame], &beginInfo), "Failed to begin command buffer");

			// begin recording commands 
			VkRenderPassBeginInfo renderpassInfo{};
			renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderpassInfo.renderPass = window.renderPass;
			renderpassInfo.framebuffer = window.swapchainFramebuffers[index];
			renderpassInfo.renderArea.extent = window.swapchainExtent;
			renderpassInfo.renderArea.offset = { 0,0 };

			VkClearValue clearColor[2];
			clearColor[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
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

			vkCmdBindDescriptorSets(m_renderCommandBuffers[Enigma::currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelinePipelineLayout.handle, 0, 1, &m_sceneDescriptorSets[Enigma::currentFrame], 0, nullptr);
			
			if (current_state) {
				if (camera->GetPosition().x != m_World->player->translation.x ||
					camera->GetPosition().z != m_World->player->translation.z
					) {
					m_World->player->moved = true;
					m_World->player->setTranslation(glm::vec3(camera->GetPosition().x, 0.1f, camera->GetPosition().z));
				}
				glm::vec3 dir = camera->GetDirection();
				dir = dir * glm::vec3(3.14, 3.14, 3.14);
				m_World->player->setRotationMatrix(glm::inverse(camera->GetCameraTransform().view));
			}

			for (const auto& model : m_World->Meshes)
			{
				vkCmdBindPipeline(m_renderCommandBuffers[Enigma::currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle);
				model->Draw(m_renderCommandBuffers[Enigma::currentFrame], m_pipelinePipelineLayout.handle, m_aabbPipeline.handle);
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

		ENIGMA_VK_CHECK(vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, m_fences[Enigma::currentFrame].handle), "Failed to submit command buffer to queue.");

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
			m_pipeline = CreateGraphicsPipeline("../resources/Shaders/vertex.vert.spv", "../resources/Shaders/fragment.frag.spv", VK_FALSE, VK_TRUE, VK_TRUE, { Enigma::sceneDescriptorLayout, Enigma::descriptorLayoutModel }, m_pipelinePipelineLayout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			m_aabbPipeline = CreateGraphicsPipeline("../resources/Shaders/vertex.vert.spv", "../resources/Shaders/line.frag.spv", VK_FALSE, VK_TRUE, VK_TRUE, { Enigma::sceneDescriptorLayout, Enigma::descriptorLayoutModel }, m_pipelinePipelineLayout, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
			window.hasResized = true;
		}

		Enigma::currentFrame = (Enigma::currentFrame + 1) % Enigma::MAX_FRAMES_IN_FLIGHT;
	}

	Pipeline Renderer::CreateGraphicsPipeline(const std::string& vertex, const std::string& fragment, VkBool32 enableBlend, VkBool32 enableDepth, VkBool32 enableDepthWrite, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, PipelineLayout& pipelinelayout, VkPrimitiveTopology topology)
	{
		ShaderModule vertexShader   = CreateShaderModule(vertex.c_str(), context.device);
		ShaderModule fragmentShader = CreateShaderModule(fragment.c_str(), context.device);

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
		//VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = topology;
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

		if (enableBlend)
		{
			blendStates[0].blendEnable = VK_TRUE;
			blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			blendStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			blendStates[0].colorBlendOp = VK_BLEND_OP_ADD;
		}
		else
		{
			blendStates[0].blendEnable = VK_FALSE;
			blendStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		VkPipelineColorBlendStateCreateInfo blendInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		blendInfo.logicOpEnable = VK_FALSE;
		blendInfo.attachmentCount = 1;
		blendInfo.pAttachments = blendStates;

		VkPipelineDepthStencilStateCreateInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthInfo.depthTestEnable = enableDepth;
		depthInfo.depthWriteEnable = enableDepthWrite;
		depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthInfo.minDepthBounds = 0.0f;
		depthInfo.maxDepthBounds = 1.0f;
		
		// Push constant
		VkPushConstantRange pushConstant{};
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(Enigma::ModelPushConstant);

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = (uint32_t)descriptorLayouts.size();
		layoutInfo.pSetLayouts = descriptorLayouts.data();
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstant;

		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkResult res = vkCreatePipelineLayout(context.device, &layoutInfo, nullptr, &layout);
		
		ENIGMA_VK_CHECK(res, "Failed to create pipeline layout");

		pipelinelayout = PipelineLayout(context.device, layout);

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
		pipelineInfo.layout = pipelinelayout.handle;
		pipelineInfo.renderPass = window.renderPass;
		pipelineInfo.subpass = 0;

		VkPipeline pipeline = VK_NULL_HANDLE;
		ENIGMA_VK_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create graphics pipeline.");

		return Pipeline(context.device, pipeline);
	}
}
