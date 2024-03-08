#pragma once
#include <Volk/volk.h>
#include "Allocator.h"
#include "../Graphics/VulkanContext.h"
#include <string>

namespace Enigma
{
	class Image
	{
		public:
			Image() noexcept = default;
			Image(const VmaAllocator& allocator, VkDevice device, VkImage image = VK_NULL_HANDLE, VkImageView imgView = VK_NULL_HANDLE, VmaAllocation allocation = VK_NULL_HANDLE) noexcept
				: m_Allocator{ allocator }, device{ device }, image { image }, imageView{ imgView }, allocation{ allocation } {}
			~Image();

			Image(const Image&) = delete;
			Image& operator=(const Image&) = delete;

			Image(Image&&) noexcept;
			Image& operator=(Image&&) noexcept;

		public:
			VkDevice device = VK_NULL_HANDLE;
			std::string name;
			VkImage	image = VK_NULL_HANDLE;
			VkImageView imageView = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;
		private:
			uint32_t mipLevels = 1;
			VmaAllocator m_Allocator = VK_NULL_HANDLE;
	};

	void Transition(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout currentLayout, VkImageLayout newLayout);
	void ImageBarrier(
		VkCommandBuffer,
		VkImage,
		VkAccessFlags,
		VkAccessFlags,
		VkImageLayout,
		VkImageLayout,
		VkPipelineStageFlags aSrcStageMask,
		VkPipelineStageFlags aDstStageMask,
		VkImageSubresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

	uint32_t CalculateMipLevels(uint32_t width, uint32_t height);
	Image CreateTexture(const VulkanContext& context, const std::string& texturePath);
	Image CreateImageTexture2D(const VulkanContext& context, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags imageaspect, uint32_t mipLevels = 1);
}