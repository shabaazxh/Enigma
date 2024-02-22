#pragma once
#include <Volk/volk.h>
#include "Allocator.h"
#include "VulkanObjects.h"
#include "VulkanContext.h"
#include "Model.h"
#include "../Core/VulkanWindow.h"

#include <vector>

namespace Enigma
{
	inline uint32_t currentFrame = 0;
	inline uint32_t max_frames_in_flight = 3;

	class Renderer
	{
		public:
			explicit Renderer(const VulkanContext& context, VulkanWindow& window);
			~Renderer();
			void DrawScene();

			void CreateGraphicsPipeline();

			Allocator allocator;
		private:
			void CreateRendererResources();
		private:
			const VulkanContext& context;
			VulkanWindow& window;

			std::vector<Fence> m_fences;
			std::vector<Semaphore> m_imagAvailableSemaphores;
			std::vector<Semaphore> m_renderFinishedSemaphores;

			std::vector<CommandPool> m_renderCommandPools;
			std::vector<VkCommandBuffer> m_renderCommandBuffers;

			Pipeline m_pipeline;
			PipelineLayout m_pipelineLayout;
			std::vector<Model*> m_renderables;
	};
}