
#include "Allocator.h"
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "../Core/Error.h"

namespace Enigma
{
	Allocator::Allocator() noexcept = default;

	Allocator::~Allocator()
	{
		//if (allocator != VK_NULL_HANDLE)
		//{
		//	vmaDestroyAllocator(allocator);
		//}
	}

	Allocator::Allocator(VmaAllocator aAllocator) noexcept : allocator{ aAllocator } {}

	Allocator::Allocator(Allocator&& other) noexcept : allocator(std::exchange(other.allocator, VK_NULL_HANDLE)) {}

	Allocator& Allocator::operator=(Allocator&& other) noexcept
	{
		std::swap(allocator, other.allocator);
		return *this;
	}

	Allocator MakeAllocator(VkInstance instance, VkPhysicalDevice pDevice, VkDevice device)
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(pDevice, &props);

		VmaVulkanFunctions functions{};
		functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocInfo{};
		allocInfo.vulkanApiVersion = props.apiVersion;
		allocInfo.physicalDevice = pDevice;
		allocInfo.device = device;
		allocInfo.instance = instance;
		allocInfo.pVulkanFunctions = &functions;

		VmaAllocator allocator = VK_NULL_HANDLE;

		ENIGMA_VK_CHECK(vmaCreateAllocator(&allocInfo, &allocator), "Failed to create allocator");

		return Allocator(allocator);
	}
}


