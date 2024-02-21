#pragma once
#include <Volk/volk.h>
#include "Allocator.h"

namespace Enigma
{
	class Buffer
	{
		public:
			Buffer() noexcept;
			~Buffer();

			explicit Buffer(VmaAllocator allocator, VkBuffer = VK_NULL_HANDLE, VmaAllocation = VK_NULL_HANDLE) noexcept;

		public:
			VkBuffer buffer = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;
		private:
			VmaAllocator m_Allocator = VK_NULL_HANDLE;
	};

	Buffer CreateBuffer(const Allocator& allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags memoryFlag, VmaMemoryUsage memoryUsage);
}