
#include "VulkanContext.h"
#include "../Core/VulkanWindow.h"

namespace ImGui
{
	
}

namespace Enigma
{
	namespace ImGuiRenderer
	{
        static std::vector<VkDescriptorPoolSize> ImGuiPoolSizes = {
            {VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}
        };

		void Initialize(const VulkanContext& context, VulkanWindow& window);
        void RenderPass(const VulkanContext& context);
		void Shutdown(const VulkanContext& context);

		void Render(VkCommandBuffer cmd, VulkanWindow& window, uint32_t index);

        inline VkDescriptorPool imGuiDescriptorPool;
        inline VkRenderPass imGuiRenderPass;
	}
}
