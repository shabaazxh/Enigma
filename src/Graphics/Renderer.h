#pragma once
#include <Volk/volk.h>
#include "Allocator.h"
#include "VulkanObjects.h"
#include "VulkanContext.h"
#include "Common.h"
#include "../Core/VulkanWindow.h"
#include <memory.h>
#include <vector>
#include "../Core/Error.h"
#include "../Core/Engine.h"
#include "../Core/World.h"

#include <memory>
#include "GBuffer.h"
#include "Lighting.h"

namespace Enigma
{
	class Renderer
	{
		public:
			explicit Renderer(const VulkanContext& context, VulkanWindow& window, Camera* camera, World& world);
			~Renderer();
			void DrawScene();
			void Update(Camera* cam);
			Pipeline CreateGraphicsPipeline(const std::string& vertex, const std::string& fragment, VkBool32 enableBlend, VkBool32 enableDepth, VkBool32 enableDepthWrite, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, PipelineLayout& pipelinelayout, VkPrimitiveTopology topology);
		private:
			void CreateRendererResources();
			void CreateDescriptorSetLayouts();
			void CreateSceneDescriptorSetLayout();
		private:
			
			GBuffer* m_gBuffer;
			Lighting* m_lighting;

			bool current_state = false;
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
			GBufferTargets gBufferTargets;
	};
}