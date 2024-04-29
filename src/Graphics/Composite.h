#pragma once
#include "GBuffer.h"

#define COMPOSITE_VERTEX "../resources/Shaders/fs_quad.vert.spv"
#define COMPOSITE_FRAGMENT "../resources/Shaders/composite.frag.spv"

class GBuffer;
class Lighting;
class VulkanContext;
class VulkanWindow;
struct GBufferTargets;
class Image;
class Buffer;
class Camera;
struct GBufferTargets;

// this should take in the lighting output render 
namespace Enigma
{
	class Composite
	{
	public:
		Composite(const VulkanContext& context, const VulkanWindow& window, Image& LightingPass);
		~Composite();

		void Execute(VkCommandBuffer cmd);
		void Resize(VulkanWindow& window);
	private:
		void CreatePipeline(VkDevice device, VkExtent2D swapchainExtent);
		void BuildDescriptorSetLayout(const VulkanContext& context);
	private:
		const VulkanContext& context;
		const VulkanWindow& window;
		uint32_t m_width;
		uint32_t m_height;
		Pipeline m_pipeline;
		PipelineLayout m_pipelineLayout;
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::vector<VkDescriptorSet> m_descriptorSets;
		Image renderTarget;
		Image& m_LightingPass;
	};
};