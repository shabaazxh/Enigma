
#include "Allocator.h"
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>


namespace Enigma
{
	Allocator::Allocator() noexcept = default;

	Allocator::~Allocator()
	{
		if (allocator != VK_NULL_HANDLE)
		{
			vmaDestroyAllocator(allocator);
		}
	}

	Allocator::Allocator(VmaAllocator aAllocator) noexcept : allocator{ aAllocator } {}

	Allocator::Allocator(Allocator&& other) noexcept : allocator(std::exchange(other.allocator, VK_NULL_HANDLE)) {}

	Allocator& Allocator::operator=(Allocator&& other) noexcept
	{
		std::swap(allocator, other.allocator);
		return *this;
	}

	Allocator MakeAllocator(const VulkanContext& context)
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(context.physicalDevice, &props);

		VmaVulkanFunctions functions{};
		functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocInfo{};
		allocInfo.vulkanApiVersion = props.apiVersion;
		allocInfo.physicalDevice = context.physicalDevice;
		allocInfo.device = context.device;
		allocInfo.instance = context.instance;
		allocInfo.pVulkanFunctions = &functions;

		VmaAllocator allocator = VK_NULL_HANDLE;

		VK_CHECK(vmaCreateAllocator(&allocInfo, &allocator));

		return Allocator(allocator);
	}
}


