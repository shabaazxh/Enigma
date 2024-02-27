#pragma once

#include <Volk/volk.h>
#include <glm/glm.hpp>
#include "VulkanContext.h"
#include "VulkanImage.h"
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
		glm::vec2 texCoords;

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
			attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributes[1].offset = offsetof(Vertex, texCoords);
			
			return attributes;
		}

	};

	class Model
	{

		public:
			Model(const std::string& filepath, const char* texpath, Allocator& aAllocator, const VulkanContext& context, VkDescriptorPool aDpool);
			void Draw(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptor, VkPipelineLayout pipelineLayout);

			Allocator& allocator;
			Enigma::DescriptorSetLayout objectLayout;
		private:
			void LoadModel();
			VkDescriptorSet alloc_desc_set(const Enigma::VulkanContext& aContext, VkDescriptorPool aPool, VkDescriptorSetLayout aSetLayout);
			Enigma::DescriptorSetLayout create_model_descriptor_layout(Enigma::VulkanContext const& aWindow);
			void update_descriptor_sets(Enigma::VulkanContext const& aWindow, VkDescriptorSet objectDescriptors);
			Sampler create_default_sampler(VulkanContext const& aContext);
			CommandPool create_command_pool(VulkanContext const&, VkCommandPoolCreateFlags = 0);
			ImageView create_image_view_texture2d(VulkanContext const&, VkImage, VkFormat);
		private:
			const VulkanContext& context;
			std::vector<Vertex> m_vertices;
			std::vector<uint32_t> m_indices;
			VkDescriptorPool dpool;
			VkDescriptorSet m_descriptors;
			Enigma::Sampler sampler;
			Enigma::ImageView objectView;
			Buffer m_vertexBuffer;
			Buffer m_indexBuffer;
			std::string m_filePath;
	};
}