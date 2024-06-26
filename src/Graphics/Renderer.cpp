#include "Renderer.h"
#include "Player.h"
#include <fstream>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <cmath>
#include <corecrt_math_defines.h>
#include "../Core/Settings.h"
#include <imgui/imgui_impl_vulkan.h>

namespace Tweakables
{
	glm::vec4 SunPosition;
	float SunInclination = 0.1f;
	float SunOrientation = 0.1f;

	float SunIntensity;
	int DebugDisplayRenderTarget = 0;

	int stepCount = 0;
	float thickness = 0.001f;
	float maxDistance = 0.005f;
}

namespace Enigma
{
	Renderer::Renderer(const VulkanContext& context, VulkanWindow& window, Camera* camera) : context{ context }, window{ window }, camera{ camera } 
	{	
		CreateRendererResources();		

        // Set = 2
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings = {
				CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			};

			Enigma::boneTransformDescriptorLayout= CreateDescriptorSetLayout(context, bindings);
		}

		m_shadowPass = new ShadowPass(context, window);
		m_gBufferPass = new GBuffer(context, window, gBufferTargets);
		m_lightingPass = new Lighting(context, window, gBufferTargets, m_shadowPass->GetRenderTarget());
		m_compositePass = new Composite(context, window, m_lightingPass->GetRenderTarget());
		m_uiPass = new UIPass(context, window);
		ImGuiRenderer::Initialize(context, window);

		// m_pipeline = CreateGraphicsPipeline("../resources/Shaders/vertex.vert.spv", "../resources/Shaders/fragment.frag.spv", VK_FALSE, VK_TRUE, VK_TRUE, { Enigma::sceneDescriptorLayout, Enigma::descriptorLayoutModel }, m_pipelinePipelineLayout, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		//m_aabbPipeline = CreateGraphicsPipeline("../resources/Shaders/vertex.vert.spv", "../resources/Shaders/line.frag.spv", VK_FALSE, VK_TRUE, VK_TRUE, { Enigma::sceneDescriptorLayout, Enigma::descriptorLayoutModel }, m_pipelinePipelineLayout, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
	}

	Renderer::~Renderer()
	{
		// Ensure all commands have finished and the GPU is now idle
		vkDeviceWaitIdle(context.device);

		delete m_shadowPass;
		delete m_lightingPass;
		delete m_gBufferPass;
		delete m_compositePass;
        delete m_uiPass;

		ImGuiRenderer::Shutdown(context);

		for (auto& model : Enigma::WorldInst.Meshes)
		{
			delete model;
		}

		// destroy the command buffers
		for (size_t i = 0; i < Enigma::MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkFreeCommandBuffers(context.device, m_renderCommandPools[i].handle, 1, &m_renderCommandBuffers[i]);
		}

		vkDestroySampler(context.device, Enigma::defaultSampler, nullptr);
		vkDestroySampler(context.device, Enigma::repeatSampler, nullptr);
		vkDestroyDescriptorSetLayout(context.device, Enigma::descriptorLayoutModel, nullptr); // this one is not being destroyed for some reason 
        vkDestroyDescriptorSetLayout(context.device, Enigma::boneTransformDescriptorLayout, nullptr);
		vkDestroyDescriptorPool(context.device, Enigma::descriptorPool, nullptr);
	}

	void Renderer::CreateDescriptorPool()
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
	}

	void Renderer::CreateRendererResources()
	{
		CreateDescriptorPool();

		Enigma::defaultSampler = Enigma::CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		Enigma::repeatSampler = Enigma::CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_REPEAT);

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

	// This will handle the settings shown in ImGui
	void Renderer::UpdateImGui()
	{	
		if (Enigma::isDebug)
		{
			ImGuiViewport* pViewport = ImGui::GetMainViewport();

			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Load Mesh", nullptr, nullptr))
					{
						if (ImGui::Begin("Mesh Loader"))
						{
							static char str[200] = "some content";
							ImGui::InputText("Input text: ", str, IM_ARRAYSIZE(str));
							if (ImGui::Button("Load"))
							{
								std::cout << "Pressed button" << std::endl;
							}
						}

						ImGui::End();
					}

					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}

			ImGui::Begin("Debug");
			ImGui::TextColored(ImVec4(0.76, 0.5, 0.0, 1.0), "FPS: (%.1f FPS), %.3f ms/frame", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
			
			
			ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)",
				camera->GetPosition().x, camera->GetPosition().y, camera->GetPosition().z);

			ImGui::Text("Camera Direction: (%.2f, %.2f, %.2f)",
				camera->GetDirection().x, camera->GetDirection().y, camera->GetDirection().z);


			ImGui::Text("Player Camera Position: (%.2f, %.2f, %.2f)",
				Enigma::WorldInst.player->GetCamera()->GetPosition().x, Enigma::WorldInst.player->GetCamera()->GetPosition().y, Enigma::WorldInst.player->GetCamera()->GetPosition().z);

			ImGui::Text("Player Camera Direction: (%.2f, %.2f, %.2f)",
				Enigma::WorldInst.player->GetCamera()->GetDirection().x, Enigma::WorldInst.player->GetCamera()->GetDirection().y, Enigma::WorldInst.player->GetCamera()->GetDirection().z);

			if (Enigma::isDebug && !Enigma::enablePlayerCamera)
			{
				ImGui::Text("DEBUG - FREE CAMERA");
			}
			else if (Enigma::isDebug && Enigma::enablePlayerCamera)
			{
				ImGui::Text("DEBUG - PLAYER CAMERA");
			}
			else {
				ImGui::Text("GAME MODE - FREE CAMERA");
			}

			if (!Enigma::WorldInst.Lights.empty())
			{
				Tweakables::SunPosition = Enigma::WorldInst.Lights[0].m_position;
				if (ImGui::CollapsingHeader("Lighting"))
				{
					float* SunPosition[3] = { &Tweakables::SunPosition.x, &Tweakables::SunPosition.y, &Tweakables::SunPosition.z };
					ImGui::SliderFloat("X: ", SunPosition[0], -300.0f, 300.0f);
					ImGui::SliderFloat("Y: ", SunPosition[1], -10.0f, 2000.0f);
					ImGui::SliderFloat("Z: ", SunPosition[2], -300.0f, 300.0f);
					ImGui::SliderFloat("Intesity: ", &Tweakables::SunIntensity, 1.0, 10.0);
					ImGui::SliderFloat("Shadow View:", &Enigma::WorldInst.Lights[0].view, 0, 2000.0f);
					ImGui::SliderFloat("Light Near: ", &Enigma::WorldInst.Lights[0].m_near, -100.0f, 10.0f);
					ImGui::SliderFloat("Light Far: ", &Enigma::WorldInst.Lights[0].m_far, 1.0f, 1000.0f);
				}

				Enigma::WorldInst.Lights[0].m_position = Tweakables::SunPosition;
				Enigma::WorldInst.Lights[0].m_intensity = Tweakables::SunIntensity;
				Enigma::WorldInst.Meshes[1]->setTranslation(glm::vec3(Tweakables::SunPosition.x, Tweakables::SunPosition.y, Tweakables::SunPosition.z));
			}

			if (ImGui::CollapsingHeader("Render Type"))
			{
				const char* types[3] = { "Final", "ShadowMap", "Normals" };
				if (ImGui::ListBox("Render Type", &Tweakables::DebugDisplayRenderTarget, types, 3))
				{
					debugSettings.debugRenderTarget = Tweakables::DebugDisplayRenderTarget;
				}
			}

			if (ImGui::CollapsingHeader("SSR"))
			{
				ImGui::SliderInt("RayCount: ", &Tweakables::stepCount, 0, 100);
				ImGui::SliderFloat("Thickness: ", &Tweakables::thickness, 0.f, 0.100f);
				ImGui::SliderFloat("MaxDist: ", &Tweakables::maxDistance, 0.f, 1.100f);

				debugSettings.stepCount = Tweakables::stepCount;
				debugSettings.thickness = Tweakables::thickness;
				debugSettings.maxDistance = Tweakables::maxDistance;
			}
			// Use the ID to uniquely move each unique mesh we have inside the meshes array
			int n = 0;
			int m = 0;
			for (const auto& model : Enigma::WorldInst.Meshes)
			{
				ImGui::PushID(n);
				if (ImGui::CollapsingHeader(model->modelName.c_str()))
				{

				/*	for (auto& mesh : model->meshes)
					{
						ImGui::PushID(m);
						if (ImGui::CollapsingHeader("mesh"))
						{
							auto pos = mesh.position;
							float* p[3] = { &pos.x, &pos.y, &pos.z };
							ImGui::SliderFloat3("Transform: ", *p, -1000.0f, 1000.0f);

							mesh.position.x = pos.x;
							mesh.position.y = pos.y;
							mesh.position.z = pos.z;

						}
						ImGui::PopID();
						m++;
					}*/
							
					auto current_position = model->getTranslation();
					auto current_scale = model->getScale();
					float* pos[3] = { &current_position.x, &current_position.y, &current_position.z};
					float* scale[3] = { &current_scale.x, &current_scale.y, &current_scale.z };
					
					if (ImGui::SliderFloat3("Transform: ", *pos, 0.0f, 20.0f))
					{
						std::cout << "Recalculating AABB" << std::endl;
					}

					ImGui::SliderFloat3("Scale: ", *scale, 0.f, 10.0);

					model->setScale(current_scale);
					model->setTranslation(current_position);
				}
				ImGui::PopID();
				n++;
			}


			if (ImGui::CollapsingHeader("Player"))
			{
				auto player_pos = Enigma::WorldInst.player->GetPosition();
				float* pos[3] = { &player_pos.x, &player_pos.y, &player_pos.z };
				ImGui::SliderFloat3("Transform: ", *pos, -20.0f, 20.0f);

				Enigma::WorldInst.player->SetPosition(player_pos);

				float fov = 45.0f;
				ImGui::SliderFloat("FOV: ", &fov, 45.0f, 90.0f);

				Enigma::WorldInst.player->GetCamera()->SetFoV(fov);

			}

			ImGui::SliderFloat("transAmp: ", &translationAmplitude, 0.0f, 100.0f);
			ImGui::SliderFloat("transFreq: ", &translationFrequency, 0.0f, 100.0f);
			ImGui::SliderFloat("rotAmp: ", &rotationAmplitude, 0.0f, 100.0f);
			ImGui::SliderFloat("rotFreq: ", &rotationFrequency, 0.0f, 100.0f);

			ImGui::End();
		}
	}

	Pipeline Renderer::CreateGraphicsPipeline(const std::string& vertex, const std::string& fragment, VkBool32 enableBlend, VkBool32 enableDepth, VkBool32 enableDepthWrite, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, PipelineLayout& pipelinelayout, VkPrimitiveTopology topology)
	{
		ShaderModule vertexShader = CreateShaderModule(vertex.c_str(), context.device);
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

	void Renderer::Update(Camera* cam)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (Enigma::isDebug && !Enigma::enablePlayerCamera)
		{
			glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else
		{
			glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		

		UpdateImGui();
		
		m_shadowPass->Update();

		if (!Enigma::enablePlayerCamera)
		{	
			window.camera = cam;
			glfwSetKeyCallback(window.window, window.glfw_callback_key_press);
			cam->Update(window.window, window.swapchainExtent.width, window.swapchainExtent.height);
			m_gBufferPass->Update(cam);
			m_lightingPass->Update(cam);
		}
		else
		{
			window.camera = Enigma::WorldInst.player->GetCamera();
			glfwSetKeyCallback(window.window, Enigma::WorldInst.player->PlayerKeyCallback);
			Enigma::WorldInst.player->Update(window.window, window.swapchainExtent.width, window.swapchainExtent.height, context, Enigma::WorldInst.Meshes, *Enigma::EngineTime);
			m_gBufferPass->Update(Enigma::WorldInst.player->GetCamera());
			m_lightingPass->Update(Enigma::WorldInst.player->GetCamera());
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

		vkResetCommandBuffer(m_renderCommandBuffers[Enigma::currentFrame], 0);

		VkCommandBuffer cmd = m_renderCommandBuffers[Enigma::currentFrame];

		// Rendering ( Record commands for submission )
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			ENIGMA_VK_CHECK(vkBeginCommandBuffer(m_renderCommandBuffers[Enigma::currentFrame], &beginInfo), "Failed to begin command buffer");

			m_shadowPass->Execute(m_renderCommandBuffers[Enigma::currentFrame], Enigma::WorldInst.Meshes);
			m_gBufferPass->Execute(m_renderCommandBuffers[Enigma::currentFrame], Enigma::WorldInst.Meshes);
			m_lightingPass->Execute(m_renderCommandBuffers[Enigma::currentFrame]);
			m_compositePass->Execute(m_renderCommandBuffers[Enigma::currentFrame]);

		    if (Enigma::enablePlayerCamera) m_uiPass->Execute(m_renderCommandBuffers[Enigma::currentFrame]);

			ImGuiRenderer::Render(cmd, window, index);
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
			gBufferTargets;
			vkDeviceWaitIdle(context.device);
			Enigma::RecreateSwapchain(context, window); 
			m_gBufferPass->Resize(window);
			m_lightingPass->Resize(window);
			m_compositePass->Resize(window);
            m_uiPass->Resize(window);
			window.hasResized = true;
		}

		Enigma::currentFrame = (Enigma::currentFrame + 1) % Enigma::MAX_FRAMES_IN_FLIGHT;
	}
}
