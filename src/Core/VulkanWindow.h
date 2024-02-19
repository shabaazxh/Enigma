#pragma once


#include "../Graphics/VulkanContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Enigma
{
	class VulkanWindow final : public VulkanContext
	{
		public:
			VulkanWindow(VulkanContext& vulkanContext);
			~VulkanWindow();

		public:
			GLFWwindow* window = nullptr;
			VkSurfaceKHR surface = VK_NULL_HANDLE;

			uint32_t presentFamilyIndex = 0;
			VkQueue presentQueue = VK_NULL_HANDLE;

			VkSwapchainKHR swapchain = VK_NULL_HANDLE;
			std::vector<VkImage> swapchainImages;
			std::vector<VkImageView> swapchainImageViews;
			std::vector<VkFramebuffer> swapchainFramebuffers;

			VkFormat swapchainFormat;
			VkExtent2D swapchainExtent;

		private:
			VulkanContext context;
	};

	VulkanWindow MakeVulkanWindow(VulkanContext& context);
}