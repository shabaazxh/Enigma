#include "VulkanWindow.h"
#include <cassert>
#include <algorithm>
#include <unordered_set>

namespace
{
	std::vector<VkSurfaceFormatKHR> GetSurfaceFormats(VkPhysicalDevice pDevice, VkSurfaceKHR surface);
	std::unordered_set<VkPresentModeKHR> GetPresentModes(VkPhysicalDevice pDevice, VkSurfaceKHR surface);

	std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D> CreateSwapchain(
		VkPhysicalDevice pDevice,
		VkSurfaceKHR surface,
		VkDevice device,
		GLFWwindow* window,
		const std::vector<uint32_t>& aQueueFamilyIndices = {},
		VkSwapchainKHR OldSwapchain = VK_NULL_HANDLE);

	std::vector<VkImage> GetSwapchainImages(VkDevice device, VkSwapchainKHR swapchain);
	std::vector<VkImageView> CreateSwapchainImageViews(VkDevice device, VkFormat format, const std::vector<VkImage>& images);
	void CreateSwapchainFramebuffers();
	std::optional<uint32_t> FindQueueFamilyIndex(VkPhysicalDevice pDevice, VkSurfaceKHR surface, VkQueueFlagBits flags);
}

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

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		windowContext.window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

		if (!windowContext.window)
			throw std::runtime_error("Failed to create GLFW window");

		if (glfwCreateWindowSurface(context.instance, windowContext.window, nullptr, &windowContext.surface) != VK_SUCCESS)
		{
			std::fprintf(stderr, "Failed to create window surface");
			std::runtime_error("Failed to create window surface");
		}

		const auto index = FindQueueFamilyIndex(context.physicalDevice, windowContext.surface, VK_QUEUE_GRAPHICS_BIT);

		std::tie(windowContext.swapchain, windowContext.swapchainFormat, windowContext.swapchainExtent) = CreateSwapchain(context.physicalDevice, windowContext.surface, context.device, windowContext.window,{context.graphicsFamilyIndex}, VK_NULL_HANDLE);
		
		windowContext.swapchainImages = GetSwapchainImages(context.device, windowContext.swapchain);
		windowContext.swapchainImageViews = CreateSwapchainImageViews(context.device, windowContext.swapchainFormat, windowContext.swapchainImages);
		
		
		return windowContext;
	}

}

namespace
{

	std::vector<VkSurfaceFormatKHR> GetSurfaceFormats(VkPhysicalDevice pDevice, VkSurfaceKHR surface)
	{
		uint32_t numFormat = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &numFormat, nullptr);

		std::vector<VkSurfaceFormatKHR> formats(numFormat);
		vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &numFormat, nullptr);

		// Ensure there are formats for us to check by checking it's not empty
		assert(!formats.empty());

		return formats;
	}

	std::unordered_set<VkPresentModeKHR> GetPresentModes(VkPhysicalDevice pDevice, VkSurfaceKHR surface)
	{

		uint32_t numModes;
		vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, surface, &numModes, nullptr);

		std::vector<VkPresentModeKHR> presentModes(numModes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice, surface, &numModes, presentModes.data());

		std::unordered_set<VkPresentModeKHR> presents;

		for (const auto& mode : presentModes)
		{
			presents.insert(mode);
		}
		
		return presents;
	}

	std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D> CreateSwapchain(VkPhysicalDevice pDevice, VkSurfaceKHR surface, VkDevice device, GLFWwindow* window, const std::vector<uint32_t>& aQueueFamilyIndices, VkSwapchainKHR OldSwapchain)
	{
		const auto formats = GetSurfaceFormats(pDevice, surface);
		const auto presentModes = GetPresentModes(pDevice, surface);

		// Find the swapchain format that we want
		VkSurfaceFormatKHR swapFormat = formats[0];
		for (const auto& format : formats)
		{
			if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				swapFormat = format;
				break;
			}


			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				swapFormat = format;
				break;
			}

		}

		// We want to use FIFO_RELAXED if it's available to use
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		if (presentModes.count(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
			presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;

		// find image count
		VkSurfaceCapabilitiesKHR caps;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pDevice, surface, &caps);

		uint32_t imageCount = 2; // double-buffering

		if (imageCount < caps.minImageCount + 1)
			imageCount = caps.minImageCount + 1;
		if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
			imageCount = caps.maxImageCount;

		VkExtent2D extent = caps.currentExtent;

		// if extent is max limiit of uint32_t, vulkan driver does not know the extent
		if (extent.width == std::numeric_limits<uint32_t>::max())
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			// Ensure extent is within defined min and max extent
			const auto& min = caps.minImageExtent;
			const auto& max = caps.maxImageExtent;

			// clamp the extent between the min and the max

			extent.width = std::clamp(uint32_t(width), min.width, max.width);
			extent.height = std::clamp(uint32_t(height), min.height, max.height);
		}

		// Provide the relevant information to make the swapchain
		VkSwapchainCreateInfoKHR swapchainInfo{};
		swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainInfo.surface = surface;
		swapchainInfo.minImageCount = imageCount;
		swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
		swapchainInfo.imageColorSpace = swapFormat.colorSpace;
		swapchainInfo.imageExtent = extent;
		swapchainInfo.imageArrayLayers = 1;
		swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainInfo.preTransform = caps.currentTransform;
		swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainInfo.presentMode = presentMode;
		swapchainInfo.clipped = VK_TRUE;
		swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

		if (aQueueFamilyIndices.size() <= 1)
		{
			swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		else
		{
			swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainInfo.queueFamilyIndexCount = uint32_t(aQueueFamilyIndices.size());
			swapchainInfo.pQueueFamilyIndices = aQueueFamilyIndices.data();
		}

		VkSwapchainKHR swapc = VK_NULL_HANDLE;

		vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapc);

		return { swapc, swapFormat.format, extent };
	}

	std::vector<VkImage> GetSwapchainImages(VkDevice device, VkSwapchainKHR swapchain)
	{
		uint32_t numImages = 0;
		vkGetSwapchainImagesKHR(device, swapchain, &numImages, nullptr);

		std::vector<VkImage> swapchainImages(numImages);
		vkGetSwapchainImagesKHR(device, swapchain, &numImages, swapchainImages.data());


		return swapchainImages;
	}

	std::vector<VkImageView> CreateSwapchainImageViews(VkDevice device, VkFormat format, const std::vector<VkImage>& images)
	{
		std::vector<VkImageView> swapchainImageViews;
		for (size_t i = 0; i < images.size(); i++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = images[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.components = VkComponentMapping{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			};

			viewInfo.subresourceRange = VkImageSubresourceRange{
				VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
			};

			VkImageView imageView = VK_NULL_HANDLE;
			VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &imageView));

			swapchainImageViews.emplace_back(imageView);
		}

		return swapchainImageViews;

	}

	std::optional<uint32_t> FindQueueFamilyIndex(VkPhysicalDevice pDevice, VkSurfaceKHR surface, VkQueueFlagBits flags)
	{
		uint32_t numQueues = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &numQueues, nullptr);

		std::vector<VkQueueFamilyProperties> families(numQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &numQueues, families.data());

		for (uint32_t i = 0; i < numQueues; i++)
		{
			const auto& family = families[i];
			VkBool32 supportPresent;
			vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, i, surface, &supportPresent);
			
			if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportPresent)
			{
				return i;
			}
		}

		return {};

	}
}