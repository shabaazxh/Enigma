#include "VulkanImage.h"
#include <cassert>

namespace Enigma
{
	Image::~Image()
	{
		if (image != VK_NULL_HANDLE)
		{
			assert(m_Allocator != VK_NULL_HANDLE);
			assert(allocation != VK_NULL_HANDLE);
			vmaDestroyImage(m_Allocator, image, allocation);
		}
	}

	Image CreateImageTexture2D(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags flags)
	{
		return Image();
	}
}

