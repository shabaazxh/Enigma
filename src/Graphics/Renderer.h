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
			explicit Renderer(const VulkanContext& context, VulkanWindow& window, Camera* camera);
			~Renderer();
			void DrawScene();
			void Update();
			Pipeline CreateGraphicsPipeline(const std::string& vertex, const std::string& fragment, VkBool32 enableBlend, VkBool32 enableDepth, VkBool32 enableDepthWrite, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, PipelineLayout& pipelinelayout, VkPrimitiveTopology topology);
		private:
			void CreateRendererResources();
			void CreateDescriptorSetLayouts();
			void CreateSceneDescriptorSetLayout();
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
			Pipeline m_aabbPipeline;
	
			PipelineLayout m_pipelinePipelineLayout;
			std::vector<VkDescriptorSet> m_sceneDescriptorSets;

			std::vector<Buffer> m_sceneUBO;
			Buffer m_SSBO;
			Image m_render;
	};
}