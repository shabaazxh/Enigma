#include "Renderer.h"

namespace
{
	Enigma::Semaphore CreateSemaphore(VkDevice device);
	Enigma::CommandPool CreateCommandPool(VkDevice device, VkCommandPoolCreateFlagBits flags, uint32_t queueFamilyIndex);
}

namespace Enigma
{
	Renderer::Renderer(const VulkanContext& context, const VulkanWindow& window) : context{ context }, window{ window } {
	
		// This will set up :
		// Fences
		// Image available and render finished semaphores
		// Command pool and command buffers;
		CreateRendererResources();
	}

	Renderer::~Renderer()
	{
		for (size_t i = 0; i < max_frames_in_flight; i++)
		{
			vkWaitForFences(context.device, 1, &m_fences[i].handle, VK_TRUE, UINT64_MAX);
			vkResetFences(context.device, 1, &m_fences[i].handle);
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

		// Check if the swapchain needs to be resized
		if (getImageIndex == VK_SUBOPTIMAL_KHR || getImageIndex == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// TODO: begin resizing the swapchain
			// ...
		}

		vkResetCommandBuffer(m_renderCommandBuffers[currentFrame], 0);

		// Rendering
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VK_CHECK(vkBeginCommandBuffer(m_renderCommandBuffers[currentFrame], &beginInfo));

			// begin recording commands 
			VkRenderPassBeginInfo renderpassInfo{};
			renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderpassInfo.renderPass = window.renderPass;
			renderpassInfo.framebuffer = window.swapchainFramebuffers[currentFrame];
			renderpassInfo.renderArea.extent = window.swapchainExtent;
			renderpassInfo.renderArea.offset = { 0,0 };

			VkClearValue clearColor = { 1.0f, 0.0f, 0.0f, 1.0f };
			renderpassInfo.clearValueCount = 1;
			renderpassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(m_renderCommandBuffers[currentFrame], &renderpassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

		VK_CHECK(vkQueuePresentKHR(context.graphicsQueue, &present));

		currentFrame = (currentFrame + 1) % max_frames_in_flight;
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
}