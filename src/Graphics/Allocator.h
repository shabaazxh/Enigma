#pragma once

#include <Volk/volk.h>
#include <vk_mem_alloc.h>
#include <utility>

namespace Enigma
{
	class Allocator
	{
		public:
			Allocator() noexcept;
			~Allocator();


			void destroy()
			{
				vmaDestroyAllocator(allocator);
			}

			explicit Allocator(VmaAllocator alloc) noexcept;

			Allocator(const Allocator&) = delete;
			Allocator& operator=(const Allocator&&) = delete;

			Allocator(Allocator&&) noexcept;
			Allocator& operator = (Allocator&&) noexcept;
		public:
			VmaAllocator allocator = VK_NULL_HANDLE;
	};

	Allocator MakeAllocator(VkInstance instance, VkPhysicalDevice pDevice, VkDevice device);
}