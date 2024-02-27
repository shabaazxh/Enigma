#pragma once
#include <Volk/volk.h>
#include "Allocator.h"
#include "VulkanObjects.h"
#include "VulkanContext.h"
#include "Model.h"
#include "Common.h"
#include "../Core/VulkanWindow.h"
#include <memory.h>
#include <vector>
#include "../Core/Error.h"

namespace Enigma
{
	class Renderer
	{
		public:
			explicit Renderer(const VulkanContext& context, VulkanWindow& window, Camera* camera, Allocator& allocator);
			~Renderer();
			void DrawScene();

			void CreateGraphicsPipeline();

			Allocator& allocator;
		private:
			void CreateRendererResources();
		private:
			World m_World;
			Camera* camera;
			const VulkanContext& context;
			VulkanWindow& window;

			std::vector<Fence> m_fences;
			std::vector<Semaphore> m_imagAvailableSemaphores;
			std::vector<Semaphore> m_renderFinishedSemaphores;

			std::vector<CommandPool> m_renderCommandPools;
			std::vector<VkCommandBuffer> m_renderCommandBuffers;

			Pipeline m_pipeline;
			PipelineLayout m_pipelineLayout;

			std::vector<VkDescriptorSet> m_sceneDescriptorSets;

			VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
			VkDescriptorSetLayout descriptorLayout_s = VK_NULL_HANDLE;
			VkDescriptorPool pool = VK_NULL_HANDLE;

			std::vector<Buffer> m_sceneUBO;
	};
}