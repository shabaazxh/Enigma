#pragma once

#include <Volk/volk.h>
#include <glm/glm.hpp>
#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "Allocator.h"
#include <string>
#include <array>
#include <vector>
#include <iostream>



#define ENIGMA_VK_ERROR(call, message) \
do { \
    VkResult result = call; \
    if (result != VK_SUCCESS) { \
        std::cout << "[ENIGMA ERROR]: " << message << " VkResult: " << result << std::endl; \
    } \
} while (0)

#define ENIGMA_ERROR(message) std::cout << "[ENIGMA ERROR]: " << message << std::endl; \

namespace Enigma
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescrip{};
			bindingDescrip.binding = 0;
			bindingDescrip.stride = sizeof(Vertex);
			bindingDescrip.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescrip;
		}

		static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> attributes{};

			attributes[0].binding = 0;
			attributes[0].location = 0;
			attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributes[0].offset = offsetof(Vertex, pos);

			attributes[1].binding = 0;
			attributes[1].location = 1;
			attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributes[1].offset = offsetof(Vertex, color);
			
			return attributes;
		}

	};

	class Model
	{

		public:
			Model(const std::string& filepath, Allocator& aAllocator, const VulkanContext& context);
			void Draw(VkCommandBuffer cmd);

			Allocator& allocator;
		private:
			void LoadModel();
		private:
			const VulkanContext& context;
			std::vector<Vertex> m_vertices;
			std::vector<uint32_t> m_indices;
			Buffer m_vertexBuffer;
			Buffer m_indexBuffer;
			std::string m_filePath;
	};
}