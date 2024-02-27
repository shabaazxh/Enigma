#pragma once

#include <Volk/volk.h>
#include <optional>
#include <iostream>
#include <vector>

namespace Enigma
{
	#define VK_CHECK(x)                                       \
		do                                                    \
		{                                                     \
			VkResult err = x;                                 \
			if (err)                                          \
			{                                                 \
				std::cout << "Vulkan error: " << err << std::endl; \
				abort();                                      \
			}                                                 \
		} while(0)  

	class VulkanContext
	{
		public:
			VulkanContext();
			~VulkanContext();

			VulkanContext(const VulkanContext&) = delete;
			VulkanContext& operator=(const VulkanContext&) = delete;

			VulkanContext(VulkanContext&&) noexcept;
			VulkanContext& operator=(VulkanContext&&) noexcept;

			VkFormat GetSupportedDepthFormat();

		public:
			VkInstance instance = VK_NULL_HANDLE;
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkDevice device = VK_NULL_HANDLE;

			uint32_t graphicsFamilyIndex = 0;
			VkQueue graphicsQueue = VK_NULL_HANDLE;
			VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	};

	VulkanContext MakeVulkanContext();
	VkInstance CreateVulkanInstance(const std::vector<const char*>& enabledLayers, const std::vector<const char*>& enabledExtensions, bool enabledDebugUtils);
	float ScoreDevice(VkPhysicalDevice pDevice);
	VkPhysicalDevice SelectDevice(VkInstance instance);
	std::optional<uint32_t> FindGraphicsQueueFamily(VkPhysicalDevice pDevice);
	VkDevice CreateDevice(VkPhysicalDevice pDevice, uint32_t graphicsFamilyIndex);
}