#include "VulkanImage.h"
#include <cassert>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace
{
	inline
		std::uint32_t countl_zero_(std::uint32_t aX)
	{
		if (!aX) return 32;

		uint32_t res = 0;

		if (!(aX & 0xffff0000)) (res += 16), (aX <<= 16);
		if (!(aX & 0xff000000)) (res += 8), (aX <<= 8);
		if (!(aX & 0xf0000000)) (res += 4), (aX <<= 4);
		if (!(aX & 0xc0000000)) (res += 2), (aX <<= 2);
		if (!(aX & 0x80000000)) (res += 1);

		return res;
	}
}

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

	Image CreateImageTexture2D(Allocator const& aAllocator, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags flags)
	{
		auto const mipLevels = compute_mip_level_count(width, height);
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = format;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = flags;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.flags = 0;
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;

		if (auto const res = vmaCreateImage(aAllocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed to allocate image\n");
		}

		return Image(aAllocator.allocator, image, allocation);
	}

	Image LoadImageTexture2D(char const* aPath, VulkanContext const& aContext, VkCommandPool aCmdPool, Allocator const& aAllocator)
	{
		// Flip images vertically by default. Vulkan expects the first scanline to be the bottom-most scanline. PNG et al.
		// instead define the first scanline to be the top-most one.
		stbi_set_flip_vertically_on_load(1);

		// Load base image
		int baseWidthi, baseHeighti, baseChannelsi;
		stbi_uc* data = stbi_load(aPath, &baseWidthi, &baseHeighti, &baseChannelsi, 4);

		if (!data)
		{
			throw std::runtime_error("Failed to load texture base image\n");
		}

		auto const baseWidth = std::uint32_t(baseWidthi);
		auto const baseHeight = std::uint32_t(baseHeighti);

		// Create staging buffer and copy image data to it
		auto const sizeInBytes = baseWidth * baseHeight * 4;
		auto staging = CreateBuffer(aAllocator, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);

		void* sptr = nullptr;
		if (auto const res = vmaMapMemory(aAllocator.allocator, staging.allocation, &sptr); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed mapping memory for writing\n");
		}

		std::memcpy(sptr, data, sizeInBytes);
		vmaUnmapMemory(aAllocator.allocator, staging.allocation);

		// Free image data
		stbi_image_free(data);

		// Create image
		Image ret = CreateImageTexture2D(aAllocator, baseWidth, baseHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		// Create command buffer for data upload and begin recording
		VkCommandBuffer cbuff = alloc_command_buffer(aContext, aCmdPool);
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (auto const res = vkBeginCommandBuffer(cbuff, &beginInfo); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed beginning command buffer recording\n");
		}

		// Transition whole image layout
		// When copying data to the image, the image’s layout must be TRANSFER DST OPTIMAL. The current
		// image layout is UNDEFINED (which is the initial layout the image was created in).
		auto const mipLevels = compute_mip_level_count(baseWidth, baseHeight);

		image_barrier(cbuff, ret.image,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, mipLevels,
				0, 1
			}
		);

		// Upload data from staging buffer to image
		VkBufferImageCopy copy;
		copy.bufferOffset = 0;
		copy.bufferRowLength = 0;
		copy.bufferImageHeight = 0;
		copy.imageSubresource = VkImageSubresourceLayers{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0,
			0, 1
		};
		copy.imageOffset = VkOffset3D{ 0, 0, 0 };
		copy.imageExtent = VkExtent3D{ baseWidth, baseHeight, 1 };
		vkCmdCopyBufferToImage(cbuff, staging.buffer, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		// Transition base level to TRANSFER SRC OPTIMAL
		image_barrier(cbuff, ret.image,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1,
				0, 1
			}
		);

		// Process all mipmap levels
		uint32_t width = baseWidth, height = baseHeight;

		for (std::uint32_t level = 1; level < mipLevels; ++level)
		{
			// Blit previous mipmap level (=level-1) to the current level. Note that the loop starts at level = 1.
			// Level = 0 is the base level that we initialied before the loop.
			VkImageBlit blit{};
			blit.srcSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1 };
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { std::int32_t(width), std::int32_t(height), 1 };

			// Next mip level
			width >>= 1; if (width == 0) width = 1;
			height >>= 1; if (height == 0) height = 1;

			blit.dstSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1 };
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { std::int32_t(width), std::int32_t(height), 1 };

			vkCmdBlitImage(cbuff, ret.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

			// Transition mip level to TRANSFER SRC OPTIMAL for the next iteration. (Technically this is
			// unnecessary for the last mip level, but transitioning it as well simplifes the final barrier following the
			// loop).
			image_barrier(cbuff, ret.image,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{
					VK_IMAGE_ASPECT_COLOR_BIT,
					level, 1,
					0, 1
				}
			);
		}

		// Whole image is currently in the TRANSFER SRC OPTIMAL layout. To use the image as a texture from
		// which we sample, it must be in the SHADER READ ONLY OPTIMAL layout.
		image_barrier(cbuff, ret.image,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VkImageSubresourceRange{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, mipLevels,
				0, 1
			}
		);

		// End command recording
		if (auto const res = vkEndCommandBuffer(cbuff); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed ending command buffer recording\n");
		}

		// Submit command buffer and wait for commands to complete. Commands must have completed before we can
		// destroy the temporary resources, such as the staging buffers.
		Enigma::Fence uploadComplete = create_fence(aContext);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cbuff;
		if (auto const res = vkQueueSubmit(aContext.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed submitting commands\n");
		}

		if (auto const res = vkWaitForFences(aContext.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed waiting for upload\n");
		}

		// Return resulting image
		// Most temporary resources are destroyed automatically through their destructors. However, the command
		// buffer we must free manually.
		vkFreeCommandBuffers(aContext.device, aCmdPool, 1, &cbuff);

		return ret;
	}

	std::uint32_t compute_mip_level_count(std::uint32_t aWidth, std::uint32_t aHeight)
	{
		std::uint32_t const bits = aWidth | aHeight;
		std::uint32_t const leadingZeros = countl_zero_(bits);
		return 32 - leadingZeros;
	}

	void image_barrier(VkCommandBuffer aCmdBuff, VkImage aImage, VkAccessFlags aSrcAccessMask, VkAccessFlags aDstAccessMask, VkImageLayout aSrcLayout, VkImageLayout aDstLayout, VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask, VkImageSubresourceRange aRange, std::uint32_t aSrcQueueFamilyIndex, std::uint32_t aDstQueueFamilyIndex) {
		VkImageMemoryBarrier ibarrier{};
		ibarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ibarrier.image = aImage;
		ibarrier.srcAccessMask = aSrcAccessMask;
		ibarrier.dstAccessMask = aDstAccessMask;
		ibarrier.srcQueueFamilyIndex = aSrcQueueFamilyIndex;
		ibarrier.dstQueueFamilyIndex = aDstQueueFamilyIndex;
		ibarrier.oldLayout = aSrcLayout;
		ibarrier.newLayout = aDstLayout;
		ibarrier.subresourceRange = aRange;

		vkCmdPipelineBarrier(aCmdBuff, aSrcStageMask, aDstStageMask, 0, 0, nullptr, 0, nullptr, 1, &ibarrier);
	}

	VkCommandBuffer alloc_command_buffer(VulkanContext const& aContext, VkCommandPool aCmdPool) 
	{
		VkCommandBufferAllocateInfo cbufInfo{};
		cbufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbufInfo.commandPool = aCmdPool;
		cbufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cbufInfo.commandBufferCount = 1;

		VkCommandBuffer cbuff = VK_NULL_HANDLE;
		if (auto const res = vkAllocateCommandBuffers(aContext.device, &cbufInfo, &cbuff); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed to allocate command buffer\n");
		}

		return cbuff;
	}

	Fence create_fence(VulkanContext const& aContext, VkFenceCreateFlags aFlags)
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = aFlags;

		VkFence fence = VK_NULL_HANDLE;
		if (auto const res = vkCreateFence(aContext.device, &fenceInfo, nullptr, &fence); VK_SUCCESS != res)
		{
			throw std::runtime_error("Failed to create fence\n");
		}

		return Fence(aContext.device, fence);
	}
}

