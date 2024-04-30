
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 

#include "Common.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "Model.h"
#include "../Core/VulkanWindow.h"
#include "../Core/World.h"

#define VERTEX "../resources/Shaders/vs_shadowpass.vert.spv"
#define FRAGMENT "../resources/Shaders/fs_shadowpass.frag.spv"


namespace Enigma
{
	class ShadowPass
	{
	public:
		ShadowPass(const VulkanContext& context, VulkanWindow& window);
		~ShadowPass();

		void Execute(VkCommandBuffer cmd, std::vector<Model*> models);
		void Update();

		// Return the render target image this pass output to
		Image& GetRenderTarget() { return renderTarget; }
	private:
		void CreateFramebuffer(VkDevice device);
		void CreateRenderPass(VkDevice device);
		void CreatePipeline(VkDevice device, VkExtent2D swapchainExtent);
		void BuildDescriptorSetLayout(const VulkanContext& context);

	private:
		const VulkanContext& context;
		VulkanWindow& window;
		uint32_t m_width;
		uint32_t m_height;
		Pipeline m_pipeline;
		PipelineLayout m_pipelineLayout;
		VkRenderPass m_RenderPass;
		VkFramebuffer m_framebuffer;
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::vector<VkDescriptorSet> m_descriptorSets;
		std::vector<Buffer> m_uniformBO;
		std::vector<Buffer> m_lightingUBO;
		Buffer m_SSBO;
		Image renderTarget;
		VkFormat m_format;
		LightUBO m_lightUBO;
	};
}