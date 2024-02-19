#include "Image.h"

namespace Enigma
{
	Image::~Image()
	{
		if (image != VK_NULL_HANDLE)
		{
			vkDestroyImage(context.device, image, nullptr);
			vkFreeMemory(context.device, memory, nullptr);
		}
	}

	Image CreateImageTexture2D(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags flags)
	{
		return Image();
	}
}

