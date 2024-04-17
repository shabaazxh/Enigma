#include "Renderer.h"
#include "Player.h"
#include <fstream>

namespace Enigma
{
	Renderer::Renderer(const VulkanContext& context, VulkanWindow& window, Camera* camera) : context{ context }, window{ window }, camera{ camera } 
	{	
		CreateRendererResources();		

		m_gBufferPass = new GBuffer(context, window, gBufferTargets);
		m_lightingPass = new Lighting(context, window, gBufferTargets);
		m_compositePass = new Composite(context, window, m_lightingPass->GetRenderTarget());

		Model* sponza = new Model("../resources/sponza_with_ship.obj", context, ENIGMA_LOAD_OBJ_FILE);
		Enigma::WorldInst.Meshes.push_back(sponza);
		Enemy* enemy1 = new Enemy("../resources/zombie-walk-test/source/Zombie_Walk1.fbx", context, ENIGMA_LOAD_FBX_FILE);
		Enigma::WorldInst.Meshes.push_back(enemy1->model);
		enemy1->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
		enemy1->setTranslation(glm::vec3(60.f, 0.1f, 0.f));
		Enigma::WorldInst.Enemies.push_back(enemy1);

		Enemy* enemy2 = new Enemy("../resources/zombie-walk-test/source/Zombie_Walk1.fbx", context, ENIGMA_LOAD_FBX_FILE);
		Enigma::WorldInst.Meshes.push_back(enemy2->model);
		enemy2->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
		enemy2->setTranslation(glm::vec3(60.f, 0.1f, 50.f));
		Enigma::WorldInst.Enemies.push_back(enemy2);

		//player = new Player(context);
		Enigma::WorldInst.player = new Player("../resources/gun.obj", context, ENIGMA_LOAD_OBJ_FILE);
		if (!Enigma::WorldInst.player->noModel) {
			Enigma::WorldInst.Meshes.push_back(Enigma::WorldInst.player->model);
		}
		Enigma::WorldInst.player->setTranslation(glm::vec3(-100.f, 0.1f, -40.f));
		Enigma::WorldInst.player->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
		Enigma::WorldInst.player->setRotationY(180);
	}

	Renderer::~Renderer()
	{
		// Ensure all commands have finished and the GPU is now idle
		vkDeviceWaitIdle(context.device);

		delete m_lightingPass;
		delete m_gBufferPass;
		delete m_compositePass;

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
		vkDestroyDescriptorSetLayout(context.device, Enigma::sceneDescriptorLayout, nullptr);
		vkDestroyDescriptorSetLayout(context.device, Enigma::descriptorLayoutModel, nullptr); // this one is not being destroyed for some reason 
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
	
		// Set = 1
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings = {
				CreateDescriptorBinding(0, 40, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			};

			Enigma::descriptorLayoutModel = CreateDescriptorSetLayout(context, bindings);
		}

	}

	void Renderer::CreateRendererResources()
	{
		CreateDescriptorPool();

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
				camera->SetPosition(Enigma::WorldInst.player->translation + glm::vec3(0.f, 13.f, 0.f));
				camera->SetNearPlane(0.05f);
			}
			else {
				camera->SetNearPlane(1.f);
			}
			current_state = isPlayer;
		}
	
		m_gBufferPass->Update(cam);
		m_lightingPass->Update(cam);
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
			
			if (current_state) {
				if (camera->GetPosition().x != Enigma::WorldInst.player->translation.x ||
					camera->GetPosition().z != Enigma::WorldInst.player->translation.z
					) {
					Enigma::WorldInst.player->moved = true;
					Enigma::WorldInst.player->setTranslation(glm::vec3(camera->GetPosition().x, 0.1f, camera->GetPosition().z));
				}
				glm::vec3 dir = camera->GetDirection();
				dir = dir * glm::vec3(3.14, 3.14, 3.14);
				Enigma::WorldInst.player->setRotationMatrix(glm::inverse(camera->GetCameraTransform().view));
			}

			m_gBufferPass->Execute(m_renderCommandBuffers[Enigma::currentFrame], Enigma::WorldInst.Meshes);
			m_lightingPass->Execute(m_renderCommandBuffers[Enigma::currentFrame]);
			m_compositePass->Execute(m_renderCommandBuffers[Enigma::currentFrame]);

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
			window.hasResized = true;
		}

		Enigma::currentFrame = (Enigma::currentFrame + 1) % Enigma::MAX_FRAMES_IN_FLIGHT;
	}
}
