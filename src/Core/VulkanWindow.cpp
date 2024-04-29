#include "VulkanWindow.h"
#include <cassert>
#include <algorithm>
#include <unordered_set>
#include "../Graphics/Common.h"

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
	std::vector<VkFramebuffer> CreateSwapchainFramebuffers(VkDevice device, std::vector<VkImageView>& swapchainImageViews, Enigma::Image& depth, VkRenderPass renderPass, VkExtent2D extent);
	VkRenderPass CreateSwapchainRenderPass(VkDevice device, VkFormat format);

	std::optional<uint32_t> FindQueueFamilyIndex(VkPhysicalDevice pDevice, VkSurfaceKHR surface, VkQueueFlagBits flags);
}

namespace Enigma
{
	VulkanWindow::VulkanWindow(const VulkanContext& context) : context{ context } {}

	VulkanWindow::~VulkanWindow()
	{
		vkDestroyImageView(context.device, Enigma::depth.imageView, nullptr);
		vmaDestroyImage(context.allocator.allocator, Enigma::depth.image, Enigma::depth.allocation);

		Enigma::depth.image = VK_NULL_HANDLE;

		for (const auto& framebuffer : swapchainFramebuffers)
			vkDestroyFramebuffer(context.device, framebuffer, nullptr);

		for (const auto& view : swapchainImageViews)
			vkDestroyImageView(context.device, view, nullptr);

		if (renderPass != VK_NULL_HANDLE)
			vkDestroyRenderPass(context.device, renderPass, nullptr);

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

	// Helper function to check if the swapchain needs  to be resized 
	bool VulkanWindow::isSwapchainOutdated(VkResult result)
	{
		// suboptimal and out of date let us know we need to re-size the swapchain
		if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) return true;
		return false;
	}

	VulkanWindow PrepareWindow(uint32_t width, uint32_t height, VulkanContext& context)
	{
		VulkanWindow windowContext(context);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		windowContext.window = glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
	
		if (!windowContext.window)
			throw std::runtime_error("Failed to create GLFW window");

		if (glfwCreateWindowSurface(context.instance, windowContext.window, nullptr, &windowContext.surface) != VK_SUCCESS)
		{
			std::fprintf(stderr, "Failed to create window surface");
		}

		return windowContext;
	}

	void MakeVulkanWindow(VulkanWindow& windowContext, VulkanContext& context, Camera* camera)
	{
		windowContext.camera = camera;

		std::tie(windowContext.swapchain, windowContext.swapchainFormat, windowContext.swapchainExtent) = CreateSwapchain(context.physicalDevice, windowContext.surface, context.device, windowContext.window,{context.presentFamilyIndex}, VK_NULL_HANDLE);
		
		uint32_t mipLevels = Enigma::CalculateMipLevels(windowContext.swapchainExtent.width, windowContext.swapchainExtent.height);
		Enigma::depth = Enigma::CreateImageTexture2D(context, windowContext.swapchainExtent.width, windowContext.swapchainExtent.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

		windowContext.swapchainImages = GetSwapchainImages(context.device, windowContext.swapchain);
		windowContext.swapchainImageViews = CreateSwapchainImageViews(context.device, windowContext.swapchainFormat, windowContext.swapchainImages);
		
		// Make swapchain render pass
		windowContext.renderPass = CreateSwapchainRenderPass(context.device, windowContext.swapchainFormat);

		windowContext.swapchainFramebuffers = CreateSwapchainFramebuffers(context.device, windowContext.swapchainImageViews, Enigma::depth, windowContext.renderPass, windowContext.swapchainExtent);
		
		windowContext.m_lastMousePosX = windowContext.swapchainExtent.width  / 2.0f;
		windowContext.m_lastMousePosY = windowContext.swapchainExtent.height / 2.0f;
	}

	void TearDownSwapchain(const VulkanContext& context, VulkanWindow& window)
	{
		vkDeviceWaitIdle(context.device);

		window.swapchainImages.clear();

		for (const auto& framebuffer : window.swapchainFramebuffers)
		{
			vkDestroyFramebuffer(context.device, framebuffer, nullptr);
		}

		for (const auto& imageView : window.swapchainImageViews)
		{
			vkDestroyImageView(context.device, imageView, nullptr);
		}

		vkDestroyRenderPass(context.device, window.renderPass, nullptr);
	}

	void RecreateSwapchain(const VulkanContext& context, VulkanWindow& window)
	{
		VkExtent2D previousExtent = window.swapchainExtent;
		VkSwapchainKHR oldSwapchain = window.swapchain;
		try
		{
			std::tie(window.swapchain, window.swapchainFormat, window.swapchainExtent) = CreateSwapchain(context.physicalDevice, window.surface, context.device, window.window, { context.graphicsFamilyIndex }, oldSwapchain);
		}
		catch ( ... ) // catch all exceptions 
		{
			window.swapchain = oldSwapchain;
			throw;
		}

		// Destroy the old swapchains resources (images, imageview, framebuffers)
		TearDownSwapchain(context, window);

		// Destroy the old swapchain
		vkDestroySwapchainKHR(context.device, oldSwapchain, nullptr);

		// Re-create new swapchain: images, imageviews, framebuffers, renderpass
		window.swapchainImages = GetSwapchainImages(context.device, window.swapchain);
		window.swapchainImageViews = CreateSwapchainImageViews(context.device, VK_FORMAT_B8G8R8A8_UNORM, window.swapchainImages);
		
		vkDestroyImageView(context.device, Enigma::depth.imageView, nullptr);
		vmaDestroyImage(context.allocator.allocator, Enigma::depth.image, Enigma::depth.allocation);
		Enigma::depth.image = VK_NULL_HANDLE;
		Enigma::depth.imageView = VK_NULL_HANDLE;
		Enigma::depth.allocation = VK_NULL_HANDLE;

		Enigma::depth = Enigma::CreateImageTexture2D(context, window.swapchainExtent.width, window.swapchainExtent.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		window.renderPass = CreateSwapchainRenderPass(context.device, VK_FORMAT_B8G8R8A8_UNORM);
		window.swapchainFramebuffers = CreateSwapchainFramebuffers(context.device, window.swapchainImageViews, Enigma::depth, window.renderPass, window.swapchainExtent);
	}
}

namespace
{
	// Query the format of the present images based on our gpu and surface
	std::vector<VkSurfaceFormatKHR> GetSurfaceFormats(VkPhysicalDevice pDevice, VkSurfaceKHR surface)
	{
		uint32_t numFormat = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &numFormat, nullptr);

		std::vector<VkSurfaceFormatKHR> formats(numFormat);
		vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice, surface, &numFormat, formats.data());

		// Ensure there are formats for us to check by checking it's not empty
		assert(!formats.empty());

		return formats;
	}

	// Query all the supported present modes given our surface and gpu 
	std::unordered_set<VkPresentModeKHR> GetPresentModes(VkPhysicalDevice pDevice, VkSurfaceKHR surface)
	{
		uint32_t numModes = 0;
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
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				swapFormat = format;
				break;
			}


			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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
		swapchainInfo.imageFormat = swapFormat.format;
		swapchainInfo.imageColorSpace = swapFormat.colorSpace;
		swapchainInfo.imageExtent = extent;
		swapchainInfo.imageArrayLayers = 1;
		swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainInfo.preTransform = caps.currentTransform;
		swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainInfo.presentMode = presentMode;
		swapchainInfo.clipped = VK_TRUE;
		swapchainInfo.oldSwapchain = OldSwapchain;

		// Set the max frames in flight using the image count we found
		Enigma::MAX_FRAMES_IN_FLIGHT = imageCount;

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
			ENIGMA_VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &imageView), "Failed to create swapchain image views");

			swapchainImageViews.emplace_back(imageView);
		}

		return swapchainImageViews;

	}

	std::vector<VkFramebuffer> CreateSwapchainFramebuffers(VkDevice device, std::vector<VkImageView>& swapchainImageViews, Enigma::Image& depth, VkRenderPass renderPass, VkExtent2D extent)
	{
		std::vector<VkFramebuffer> framebuffers;
		for (const auto& imageView : swapchainImageViews)
		{
			VkImageView attachments[1]{ imageView };

			VkFramebufferCreateInfo fb_info{};
			fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fb_info.renderPass = renderPass;
			fb_info.attachmentCount = static_cast<uint32_t>(1);
			fb_info.pAttachments = attachments;
			fb_info.width = extent.width;
			fb_info.height = extent.height;
			fb_info.layers = 1;

			VkFramebuffer framebuffer = VK_NULL_HANDLE;
			ENIGMA_VK_CHECK(vkCreateFramebuffer(device, &fb_info, nullptr, &framebuffer), "Failed to create swapchain framebuffers.");

			framebuffers.push_back(framebuffer);
		}

		return framebuffers;
	}

	VkRenderPass CreateSwapchainRenderPass(VkDevice device, VkFormat format)
	{
		VkAttachmentDescription attachment{};
		attachment.format = format;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear the framebuffer before writing to it 
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store the result
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // not using stencil, use don't care
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // not using stencil, use don't care
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout of the resource as it enters render pass
		attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout of resource at the end of the render pass

		// we need depth attachment, leaving it for now
		//VkAttachmentDescription depthAttachment{};
		//depthAttachment.format = VK_FORMAT_D32_SFLOAT;
		//depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		//depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		//depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		//depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription attachments[1] = { attachment };

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		//VkAttachmentReference depthRefernece = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;
		//subpass.pDepthStencilAttachment = &depthRefernece;
		
		VkSubpassDependency dependency[1]{};
		// Wait for EXTERNAL render pass to finish outputting
		dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency[0].srcAccessMask = 0;
		dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		// Destination needs to wait for EXTERNAL to finishing outputting before writing 
		dependency[0].dstSubpass = 0;
		dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		
		//dependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		//dependency[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		//dependency[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		//dependency[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		//dependency[1].dstSubpass = 0;
		//dependency[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		//dependency[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		VkRenderPassCreateInfo rp_info{};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.attachmentCount = 1;
		rp_info.pAttachments = attachments;
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;
		rp_info.dependencyCount = 1;
		rp_info.pDependencies = dependency;

		VkRenderPass renderPass = VK_NULL_HANDLE;

		ENIGMA_VK_CHECK(vkCreateRenderPass(device, &rp_info, nullptr, &renderPass), "Failed to create swapchain Render Pass.");

		return renderPass;
	}

}