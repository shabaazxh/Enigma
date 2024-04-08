#pragma once
#include "Common.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "Model.h"
#include "../Core/VulkanWindow.h"

#define VERTEX "../resources/Shaders/vertex.vert.spv"
#define FRAGMENT "../resources/Shaders/gbuffer.frag.spv"

namespace Enigma
{
	struct GBufferTargets
	{
		Image normals;
		Image depth;
	};
	class GBuffer
	{
	public:
		GBuffer(const VulkanContext& context, const VulkanWindow& window);
		~GBuffer();

		void Execute(VkCommandBuffer cmd, const std::vector<Model*>& models);
		void Update();
		void Resize();
	private:
		void CreateFramebuffer(VkDevice device, uint32_t width, uint32_t height);
		void CreateRenderPass(VkDevice device);
		void CreatePipeline(VkDevice device, VkExtent2D swapchainExtent);
		void BuildDescriptorSetLayout(const VulkanContext& context);
	private:
		const VulkanContext& context;
		Pipeline m_pipeline;
		PipelineLayout m_pipelineLayout;
		uint32_t m_width;
		uint32_t m_height;
		VkRenderPass m_RenderPass;
		VkFramebuffer m_framebuffer;
		std::vector<VkDescriptorSet> m_sceneDescriptorSets;
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::vector<Buffer> m_sceneUBO;
		Buffer m_SSBO;
		GBufferTargets g_BufferOutputTargets; // TODO: class owns this for now but needs to be accessible outside it 
	};

	//inline GBufferTargets g_BufferOutputTargets; // this is not being released
};
