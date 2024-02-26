#include "Model.h"
#include "VulkanObjects.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace Enigma
{
	Model::Model(const std::string& filepath, Allocator& aAllocator, const VulkanContext& context) : m_filePath{filepath}, allocator{ aAllocator }, context{context}
	{
		LoadModel();
	}

	void Model::LoadModel()
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, m_filePath.c_str()))
		{
			ENIGMA_ERROR("Failed to load model.");
		}

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};

				// Get the triangle 
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				// give it a pre-defiend color (using white)
				vertex.color = { 1.0f, 1.0f, 1.0f };

				m_vertices.push_back(vertex);

				// We should be using index rendering, but this required a little more work
				// so I have left it for now so we just use all vertices of the model
				// While I am not using indices, im still adding values to it because I create a index buffer
				// which requires data to show how it could work.
				m_indices.push_back(3);
			}
		}

		// Define the size of the buffers. Buffer will store size of the first element in bytes * amount
		VkDeviceSize vertexSize = sizeof(m_vertices[0]) * m_vertices.size();
		VkDeviceSize indexSize = sizeof(m_indices[0] * m_indices.size());

		m_vertexBuffer = CreateBuffer(allocator, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
		m_indexBuffer  = CreateBuffer(allocator, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

		// We can't directly add data to GPU memory
		// need to add data to CPU visible memory first and then copy into GPU memory
		Buffer stagingBuffer = CreateBuffer(allocator, vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);

		void* data = nullptr;		
		ENIGMA_VK_ERROR(vmaMapMemory(allocator.allocator, stagingBuffer.allocation, &data), "Failed to map staging buffer memory while loading model.");
		std::memcpy(data, m_vertices.data(), vertexSize);
		vmaUnmapMemory(allocator.allocator, stagingBuffer.allocation);

		// Transfering CPU visible data to GPU ( we need a fence to sync ) 
		VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VkFence fence = VK_NULL_HANDLE;
		VkResult res = vkCreateFence(context.device, &fenceInfo, nullptr, &fence);

		Fence uploadComplete = Fence(context.device, fence);
		vkResetFences(context.device, 1, &uploadComplete.handle);
		
		// Need a command pool & command buffer to record and submit 
		// to do copy transfer operation
		VkCommandPoolCreateInfo cmdPool{};
		cmdPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPool.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		cmdPool.queueFamilyIndex = context.graphicsFamilyIndex;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		ENIGMA_VK_ERROR(vkCreateCommandPool(context.device, &cmdPool, nullptr, &commandPool), "Failed to create command pool for staging buffer in model class");

		VkCommandBufferAllocateInfo cmdAlloc{};
		cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAlloc.commandPool = commandPool;
		cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdAlloc.commandBufferCount = 1;

		VkCommandBuffer cmd = VK_NULL_HANDLE;
		ENIGMA_VK_ERROR(vkAllocateCommandBuffers(context.device, &cmdAlloc, &cmd), "Failed to allocate command buffers while loading model.");

		// begin recording command buffers to do the copying from CPU to GPU buffer
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		ENIGMA_VK_ERROR(vkBeginCommandBuffer(cmd, &beginInfo), "Failed to start command buffer for copying operation while loading model.");

		// Specify the copy position
		VkBufferCopy copy{};
		copy.size = vertexSize;

		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, m_vertexBuffer.buffer, 1, &copy);

		VkBufferMemoryBarrier bufferBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
		bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufferBarrier.buffer = m_vertexBuffer.buffer;
		bufferBarrier.size = VK_WHOLE_SIZE;
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

		ENIGMA_VK_ERROR(vkEndCommandBuffer(cmd), "Failed to end command buffer during staging to GPU buffer opertion");

		// once the copy has finished, begin submitting
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;

		ENIGMA_VK_ERROR(vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, uploadComplete.handle), "Failed to submit transfer operation for staging to GPU while loading model");

		// Wait for copy to complete before destroying the temporary resources
		// Use fence signal to check if has completed or not 
		ENIGMA_VK_ERROR(vkWaitForFences(context.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<uint64_t>::max()), "Fence failed to wait while loading model.");

		vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
		vkDestroyCommandPool(context.device, commandPool, nullptr);
 	}

	// Call to draw the model
	void Model::Draw(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptor, VkPipelineLayout pipelineLayout)
	{
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &sceneDescriptor, 0, nullptr);
		//Todo: update with objectsecriptor
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &sceneDescriptor, 0, nullptr);

		VkDeviceSize offset[2] = {};

		vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer.buffer, offset);

		vkCmdDraw(cmd, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
	}

}

