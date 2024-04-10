#pragma once
#include "GBuffer.h"

#define LIGHTING_VERTEX "../resources/Shaders/fs_quad.vert.spv"
#define LIGHTING_FRAGMENT "../resources/Shaders/fs_quad.frag.spv"

namespace Enigma
{
	class Lighting
	{
	public:
		Lighting(const VulkanContext& context, const VulkanWindow& window, GBufferTargets& targets);
		~Lighting();

		void Execute(VkCommandBuffer cmd);
		void Update(Camera* camera);
		void Resize();
	private:
		void CreateFramebuffer(VkDevice device);
		void CreateRenderPass(VkDevice device);
		void CreatePipeline(VkDevice device, VkExtent2D swapchainExtent);
		void BuildDescriptorSetLayout(const VulkanContext& context, GBufferTargets& gBufferTargets);
	private:
		const VulkanContext& context;
		uint32_t m_width;
		uint32_t m_height;
		Pipeline m_pipeline;
		PipelineLayout m_pipelineLayout;
		VkRenderPass m_RenderPass;
		VkFramebuffer m_framebuffer;
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::vector<VkDescriptorSet> m_descriptorSets;
		std::vector<Buffer> m_uniformBO;
		Buffer m_SSBO;

		Image renderTarget;
	};
};