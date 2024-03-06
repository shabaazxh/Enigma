#pragma once

#include <Volk/volk.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "../Core/Error.h"
#include "Allocator.h"
#include <string>
#include <array>
#include <vector>
#include <iostream>
#include "../Graphics/VulkanImage.h"

namespace Enigma
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 tex;
		glm::vec3 color;

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescrip{};
			bindingDescrip.binding = 0;
			bindingDescrip.stride = sizeof(Vertex);
			bindingDescrip.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescrip;
		}

		static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 3> attributes{};

			attributes[0].binding = 0;
			attributes[0].location = 0;
			attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributes[0].offset = offsetof(Vertex, pos);

			attributes[1].binding = 0;
			attributes[1].location = 1;
			attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributes[1].offset = offsetof(Vertex, tex);

			attributes[2].binding = 0;
			attributes[2].location = 2;
			attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributes[2].offset = offsetof(Vertex, color);
			
			return attributes;
		}

	};

	struct Material
	{
		std::string materialName;
		glm::vec3 diffuseColour;
		std::string diffuseTexturePath;
	};
	
	struct Mesh
	{
		std::string meshName;
		size_t materialIndex;
		bool textured : 1;
		size_t vertexStartIndex;
		size_t vertexCount;
		
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<glm::vec2> texcoords;
		std::vector<glm::vec3> bounding_box;
		Buffer vertexBuffer;
		Buffer indexBuffer;
	};

	struct ModelPushConstant
	{
		glm::mat4 model;
		int textureIndex;
	};

	class Model
	{
		public:
			Model(const std::string& filepath, Allocator& aAllocator, const VulkanContext& context);
			void Draw(VkCommandBuffer cmd, VkPipelineLayout layout);

			Allocator& allocator;
			std::vector<Material> materials;
			std::vector<Mesh> meshes;
			std::vector<Image> loadedTextures;

			//transformations
			glm::vec3 translation = glm::vec3(0.f, 0.f, 0.f);
			float rotationX = 0.f;
			float rotationY = 0.f;
			float rotationZ = 0.f;
			glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);
		private:
			void LoadModel(const std::string& filepath);
			void CreateBuffers();
			void GetBoundingBox();
		private:
			
			std::vector<VkDescriptorSet> m_descriptorSet;
			const VulkanContext& context;
			std::string m_filePath;
	};
}