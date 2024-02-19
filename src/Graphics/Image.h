#pragma once
#include <Volk/volk.h>
#include <vk_mem_alloc.h>
#include "VulkanContext.h"

namespace Enigma
{
	class Image
	{
		public:
			Image() = default;
			Image(const VulkanContext& context) : context{ context } {}
			~Image();

		public:
			VkImage	image = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;

		private:
			VulkanContext context;
			VmaAllocator m_Allocator = VK_NULL_HANDLE;
	};

	Image CreateImageTexture2D(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags flags);
}