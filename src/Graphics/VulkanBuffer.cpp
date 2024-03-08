#include "VulkanBuffer.h"
#include <cassert>
#include "../Core/Error.h"
#include "../Core/Engine.h"

namespace Enigma
{

	Buffer::Buffer() noexcept = default;

	Buffer::~Buffer()
	{
		if (buffer != VK_NULL_HANDLE)
		{
			assert(m_Allocator != VK_NULL_HANDLE);
			assert(allocation  != VK_NULL_HANDLE);
			vmaDestroyBuffer(m_Allocator, buffer, allocation);
		}
	}

	Buffer::Buffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation) noexcept
		: buffer{ buffer }, allocation{ allocation }, m_Allocator{ allocator }
	{}

	Buffer::Buffer(Buffer&& aOther) noexcept :
		buffer(std::exchange(aOther.buffer, VK_NULL_HANDLE)),
		allocation(std::exchange(aOther.allocation, VK_NULL_HANDLE)),
		m_Allocator(std::exchange(aOther.m_Allocator, VK_NULL_HANDLE)) {}

	Buffer& Buffer::operator=(Buffer&& aOther) noexcept
	{
		std::swap(buffer, aOther.buffer);
		std::swap(allocation, aOther.allocation);
		std::swap(m_Allocator, aOther.m_Allocator);

		return *this;
	}

	Buffer CreateBuffer(const Allocator& allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags memoryFlag, VmaMemoryUsage memoryUsage)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.flags = memoryFlag;
		allocInfo.usage = memoryUsage;

		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;

		ENIGMA_VK_CHECK(vmaCreateBuffer(allocator.allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr), "Failed to create buffer");

		return Buffer(allocator.allocator, buffer, allocation);
	}
}