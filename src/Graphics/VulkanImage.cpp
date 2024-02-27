#include "VulkanImage.h"
#include <cassert>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "../Core/Error.h"
#include "../Graphics/VulkanBuffer.h"
#include "../Graphics/Common.h"
#include <algorithm>

namespace Enigma
{
	Image::~Image()
	{
		if (image != VK_NULL_HANDLE)
		{
			assert(m_Allocator != VK_NULL_HANDLE);
			assert(allocation != VK_NULL_HANDLE);
			vmaDestroyImage(m_Allocator, image, allocation);
			// need to destroy image view
		}
	}

	Image::Image(Image&& other) noexcept :
		image(std::exchange(other.image, VK_NULL_HANDLE)),
		imageView(std::exchange(other.imageView, VK_NULL_HANDLE)),
		m_Allocator(std::exchange(other.m_Allocator, VK_NULL_HANDLE)),
		allocation(std::exchange(other.allocation, VK_NULL_HANDLE)),
		name(std::exchange(other.name, "")) {}

	Image& Image::operator=(Image&& other) noexcept
	{
		std::swap(image, other.image);
		std::swap(allocation, other.allocation);
		std::swap(m_Allocator, other.m_Allocator);
		std::swap(imageView, other.imageView);

		return *this;
	}

	void Transition(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout currentLayout, VkImageLayout newLayout)
	{

		VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = currentLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;


		//vkCmdPipelineBarrier(cmd, )


	}

	void ImageBarrier(VkCommandBuffer cmd, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout srcLayout, VkImageLayout dstLayout, VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask, VkImageSubresourceRange range, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex)
	{
		VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = srcLayout;
		barrier.newLayout = dstLayout;
		barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		barrier.image = image;
		barrier.subresourceRange = range;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;

		vkCmdPipelineBarrier(cmd, aSrcStageMask, aDstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	Image CreateTexture(const VulkanContext& context, const std::string& texturePath, Allocator& allocator)
	{
		int width, height, texChannels;

		stbi_uc* pixels = stbi_load(texturePath.c_str(), &width, &height, &texChannels, 4);

		const auto imageSize = width * height * 4; // size in bytes

		if (!pixels)
		{
			ENIGMA_ERROR("Failed to load texture: ", texturePath);
		}

		Buffer stagingBuffer = Enigma::CreateBuffer(allocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* data = nullptr;
		ENIGMA_VK_ERROR(vmaMapMemory(allocator.allocator, stagingBuffer.allocation, &data), "Failed to map memory for texture: " + texturePath);
		memcpy(data, pixels, imageSize);
		vmaUnmapMemory(allocator.allocator, stagingBuffer.allocation);

		stbi_image_free(pixels);

		Image image = Enigma::CreateImageTexture2D(context, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT, allocator);
		// create image

		image.name = texturePath;

		VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolInfo.queueFamilyIndex = context.graphicsFamilyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

		VkCommandPool pool = VK_NULL_HANDLE;
		ENIGMA_VK_ERROR(vkCreateCommandPool(context.device, &poolInfo, nullptr, &pool), "Failed to allocate command pool");
		
		VkCommandBuffer cmd = Enigma::AllocateCommandBuffer(context, pool);

		Enigma::BeginCommandBuffer(cmd);

		// Transition image from layout undefined to dst_layout to do our transfer operation
		// srcaccess mask is 0
		// dstcces mask is write_bit
		ImageBarrier(cmd, image.image, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

		// upload the data from the staging buffer to the image
		VkBufferImageCopy copy{};
		copy.bufferOffset = 0;
		copy.bufferRowLength = 0;
		copy.bufferImageHeight = 0;
		copy.imageSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copy.imageOffset = VkOffset3D{ 0,0,0 };
		copy.imageExtent = VkExtent3D{ (uint32_t)width, (uint32_t)height, 1 };

		// copy the buffer contents to the image. The fourth argument is the layout of the current image
		vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		// Transition image layout from dst_optimal to shader_read_only optimal to access the image/texture in the shader
		// srcaccess mask is write bit so we wait on the writing to finish
		// dstccess is shader read bit so shader reading has to wait on transfer writing to complete before reading from it 
		ImageBarrier(cmd, image.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		// Submit commands 
		Enigma::EndAndSubmitCommandBuffer(context, cmd);

		vkFreeCommandBuffers(context.device, pool, 1, &cmd);

		return image;
	}
	//
	// VK_IMAGE_ASPECT_COLOR_BIT
	Image CreateImageTexture2D(const VulkanContext& context, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags imageaspect, Allocator& allocator)
	{
		VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.flags = 0;
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;

		ENIGMA_VK_ERROR(vmaCreateImage(allocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr), "Failed to allocate memory for image");
		
		// Now create the image view
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.components = VkComponentMapping{};
		viewInfo.subresourceRange = VkImageSubresourceRange{ imageaspect, 0, VK_REMAINING_MIP_LEVELS, 0, 1 };

		VkImageView imageView = VK_NULL_HANDLE;
		ENIGMA_VK_ERROR(vkCreateImageView(context.device, &viewInfo, nullptr, &imageView), "Failed to create image view");

		return Image(allocator.allocator, image, imageView, allocation);
	}
}

