#pragma once
#include <Volk/volk.h>
#include "VulkanObjects.h"
#include "VulkanContext.h"
#include "../Core/VulkanWindow.h"

#include <vector>

namespace Enigma
{
	inline uint32_t currentFrame = 0;
	inline uint32_t max_frames_in_flight = 3;

	class Renderer
	{
		public:
			explicit Renderer(const VulkanContext& context, const VulkanWindow& window);
			~Renderer();
			void DrawScene();

		private:
			void CreateRendererResources();
		private:
			const VulkanContext& context;
			const VulkanWindow& window;


			// need fences and semaphores for sync

			std::vector<Fence> m_fences;
			std::vector<Semaphore> m_imagAvailableSemaphores;
			std::vector<Semaphore> m_renderFinishedSemaphores;

			std::vector<CommandPool> m_renderCommandPools;
			std::vector<VkCommandBuffer> m_renderCommandBuffers;

	};
}