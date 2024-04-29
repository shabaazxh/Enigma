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
#include "../Core/Engine.h"
#include "../Core/World.h"

#include <memory>
#include "GBuffer.h"
#include "Lighting.h"
#include "Composite.h"
#include "ShadowPass.h"
#include "ImGuiRenderer.h"


namespace Enigma
{
	class Renderer
	{
		public:
			explicit Renderer(const VulkanContext& context, VulkanWindow& window, Camera* camera);
			~Renderer();
			void DrawScene();
			void Update(Camera* cam);
			Pipeline CreateGraphicsPipeline(const std::string& vertex, const std::string& fragment, VkBool32 enableBlend, VkBool32 enableDepth, VkBool32 enableDepthWrite, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, PipelineLayout& pipelinelayout, VkPrimitiveTopology topology);
		private:
			void CreateRendererResources();
			void CreateDescriptorSetLayouts();
			void CreateSceneDescriptorSetLayout();

            void CreateBlood();
            void UpdateBlood(float value);
            void DrawBlood(VkCommandBuffer cmdBuffer);

            void CreateBloodPipeline();
            void CreateBloodVertexBuffer();
            void CreateHeadPipeline();
            void CreateHeadImage();

		private:
			bool current_state = false;
			Camera* camera;
			const VulkanContext& context;
			VulkanWindow& window;

			GBuffer* m_gBufferPass;
			GBufferTargets gBufferTargets;

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

            //blood
            std::vector<glm::vec2> bloodVertices;
		    Buffer bloodVertexBuffer;
            glm::mat4 bloodTransformMatrix;
			Pipeline m_bloodPipeline;
			Pipeline m_bloodPipeline2;
			PipelineLayout m_bloodPipelineLayout;

            Image m_headImage;
            VkDescriptorSetLayout m_headDescriptorSetLayout;
            VkDescriptorSet m_headDescriptorSet;
            glm::mat4 m_headTransformMatrix;
			PipelineLayout m_headPipelineLayout;
			Pipeline m_headPipeline;
	};
}