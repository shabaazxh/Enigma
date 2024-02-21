#pragma once

#include <Volk/volk.h>
#include <vk_mem_alloc.h>
#include <utility>

#include "VulkanContext.h"

namespace Enigma
{
	class Allocator
	{
		public:
			Allocator() noexcept;
			~Allocator();

			explicit Allocator(VmaAllocator alloc) noexcept;

			Allocator(const Allocator&) = delete;
			Allocator& operator=(const Allocator&&) = delete;

			Allocator(Allocator&&) noexcept;
			Allocator& operator = (Allocator&&) noexcept;
		public:
			VmaAllocator allocator = VK_NULL_HANDLE;
	};

	Allocator MakeAllocator(const VulkanContext& context);
}