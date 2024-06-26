#pragma once

#include "Common.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "Model.h"
#include "../Core/VulkanWindow.h"
#include "../Core/World.h"

#define VERTEX "../resources/Shaders/vertex.vert.spv"
#define VERTEX_ANIM "../resources/Shaders/vertexAnim.vert.spv"
#define FRAGMENT "../resources/Shaders/gbuffer.frag.spv"

namespace Enigma
{
	class GBuffer
	{
	public:
		GBuffer(const VulkanContext& context, const VulkanWindow& window, GBufferTargets& targets);
		~GBuffer();

		void Execute(VkCommandBuffer cmd, const std::vector<Model*>& models);
		void Update(Camera* camera);
		void Resize(const VulkanWindow& window);
	private:
		void CreateFramebuffer(VkDevice device, GBufferTargets& targets);
		void CreateRenderPass(VkDevice device);
		void CreatePipeline(VkDevice device, VkExtent2D swapchainExtent);
		void CreatePipelineAnim(VkDevice device, VkExtent2D swapchainExtent);
		void CreateAABBPipeline(VkDevice device, VkExtent2D swapchainExtent);
		void BuildDescriptorSetLayout(const VulkanContext& context);
	private:
		const VulkanContext& context;
		Pipeline m_pipeline;
		Pipeline AABBDraw;
		PipelineLayout m_pipelineLayout;
		uint32_t m_width;
		uint32_t m_height;
		VkRenderPass m_RenderPass;
		VkFramebuffer m_framebuffer;
		std::vector<VkDescriptorSet> m_sceneDescriptorSets;
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::vector<Buffer> m_sceneUBO;
		GBufferTargets& targets;

		Pipeline m_pipelineAnim;
		PipelineLayout m_pipelineAnimLayout;
	};
};
