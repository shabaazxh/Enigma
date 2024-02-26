#pragma once
#include <Volk/volk.h>
#include "VulkanBuffer.h"
#include "VulkanObjects.h"

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

	Image CreateImageTexture2D(Allocator const& aAllocator, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags flags);

	Image LoadImageTexture2D(char const* aPath, VulkanContext const&, VkCommandPool, Allocator const&);

	std::uint32_t compute_mip_level_count(std::uint32_t aWidth, std::uint32_t aHeight);

	void image_barrier(VkCommandBuffer, VkImage, VkAccessFlags aSrcAccessMask, VkAccessFlags aDstAccessMask, VkImageLayout aSrcLayout, VkImageLayout aDstLayout, VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask, VkImageSubresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT ,0,1,0,1 }, std::uint32_t aSrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, std::uint32_t aDstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

	VkCommandBuffer alloc_command_buffer(VulkanContext const&, VkCommandPool);

	Fence create_fence(VulkanContext const& aContext, VkFenceCreateFlags = 0);
}