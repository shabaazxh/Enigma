#include "VulkanWindow.h"


namespace Enigma
{
	VulkanWindow::VulkanWindow(VulkanContext& context) : context{ context } {}

	VulkanWindow::~VulkanWindow()
	{
		for (const auto& view : swapchainImageViews)
			vkDestroyImageView(context.device, view, nullptr);

		if (swapchain != VK_NULL_HANDLE)
			vkDestroySwapchainKHR(context.device, swapchain, nullptr);
		
		if (surface != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(context.instance, surface, nullptr);

		if (window)
		{
			glfwDestroyWindow(window);
			glfwTerminate();
		}
	}

	VulkanWindow MakeVulkanWindow(VulkanContext& context)
	{
		VulkanWindow windowContext(context);
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		windowContext.window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
	
	
		return windowContext;
	}

}

namespace
{

	//std::vector<VkSurfaceFormatKHR> GetSurfaceFormats(VkPhysicalDevice pDevice, VkSurfaceKHR surface)
	//{
	//	uint32_t numFormat = 0;
	//	vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &numFormat, nullptr);

	//	std::vector<VkSurfaceFormatKHR> formats(numFormat);
	//	vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &numFormat, nullptr);


	//}

}