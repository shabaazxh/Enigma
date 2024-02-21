#pragma once
#include <Volk/volk.h>
#include "Allocator.h"

namespace Enigma
{
	class Image
	{
		public:
			Image() noexcept = default;
			Image(const VmaAllocator& allocator, VkImage image = VK_NULL_HANDLE, VmaAllocation allocation = VK_NULL_HANDLE) noexcept
				: m_Allocator{ allocator }, allocation{ allocation }, image{ image } {}
			~Image();

		public:
			VkImage	image = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;

		private:
			VmaAllocator m_Allocator = VK_NULL_HANDLE;
	};

	Image CreateImageTexture2D(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags flags);
}