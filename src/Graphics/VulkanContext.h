#pragma once

#include <Volk/volk.h>
#include <optional>
#include <iostream>
#include <vector>
#include "Allocator.h"
#include "../Core/Error.h"

namespace Enigma
{
	class VulkanContext
	{
		public:
			VulkanContext();
			~VulkanContext();

			VulkanContext(const VulkanContext&) = delete;
			VulkanContext& operator=(const VulkanContext&) = delete;

			VulkanContext(VulkanContext&&) noexcept;
			VulkanContext& operator=(VulkanContext&&) noexcept;

		public:
			VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
			Allocator allocator;
			VkInstance instance = VK_NULL_HANDLE;
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkDevice device = VK_NULL_HANDLE;
			VkSurfaceKHR m_surface = VK_NULL_HANDLE;
			
			uint32_t graphicsFamilyIndex = 0;
			uint32_t presentFamilyIndex = 0;
			VkQueue graphicsQueue = VK_NULL_HANDLE;
			VkQueue presentQueue = VK_NULL_HANDLE;

			bool enabledDebugUtils = false;
	};

	void MakeVulkanContext(VulkanContext& context, VkSurfaceKHR surface);
	VulkanContext PrepareContext();
	VkInstance CreateVulkanInstance(const std::vector<const char*>& enabledLayers, const std::vector<const char*>& enabledExtensions, bool enabledDebugUtils);
	float ScoreDevice(VkPhysicalDevice pDevice);
	VkPhysicalDevice SelectDevice(VkInstance instance);
	std::pair<std::optional<uint32_t>, std::optional<uint32_t>> FindGraphicsQueueFamily(VkPhysicalDevice pDevice, VkInstance instance, VkSurfaceKHR surface);
	VkDevice CreateDevice(VkPhysicalDevice pDevice, uint32_t graphicsFamilyIndex);
}