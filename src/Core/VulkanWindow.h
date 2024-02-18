#pragma once

#include <GLFW/glfw3.h>
#include "../Graphics/VulkanContext.h"
#include <unordered_set>

namespace
{
	std::vector<VkSurfaceFormatKHR> GetSurfaceFormats(VkPhysicalDevice pDevice, VkSurfaceKHR surface);
	std::unordered_set<VkPresentModeKHR> GetPresentModes(VkPhysicalDevice pDevice, VkSurfaceKHR);

	std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D> CreateSwapchain();
}

namespace Enigma
{
	class VulkanWindow
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

			VkFormat swapchainFormat;
			VkExtent2D swapchainExtent;

		private:
			VulkanContext context;
	};

	VulkanWindow MakeVulkanWindow(VulkanContext& context);
}