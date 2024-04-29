#pragma once

#include <Volk/volk.h>
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "../Core/Error.h"
#include "Allocator.h"
#include <string>
#include <array>
#include <vector>
#include <iostream>
#include "../Graphics/VulkanImage.h"
#include <functional>
#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <assimp/Importer.hpp>      
#include <assimp/scene.h>           
#include <assimp/postprocess.h>

#define ENIGMA_LOAD_OBJ_FILE 0
#define ENIGMA_LOAD_FBX_FILE 1

namespace Enigma
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
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

		static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 4> attributes{};

			attributes[0].binding = 0;
			attributes[0].location = 0;
			attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributes[0].offset = offsetof(Vertex, pos);

			attributes[1].binding = 0;
			attributes[1].location = 1;
			attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributes[1].offset = offsetof(Vertex, normal);

			attributes[2].binding = 0;
			attributes[2].location = 2;
			attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributes[2].offset = offsetof(Vertex, tex);

			attributes[3].binding = 0;
			attributes[3].location = 3;
			attributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributes[3].offset = offsetof(Vertex, color);
			
			return attributes;
		}

		bool operator==(const Vertex& other) const
		{
			return pos == other.pos && tex == other.tex;
		}

	};
	
	struct AABB
	{
		glm::vec3 min = glm::vec3(1.0f);
		glm::vec3 max = glm::vec3(1.0f);
	};

	struct Material
	{
		std::string materialName;
		glm::vec3 diffuseColour;
		std::string diffuseTexturePath;
		std::string metallicTexturePath;
		std::string roughnessTexturePath;
		//std::string normalaMapTexture;
	};
	
	struct Mesh
	{
		std::string meshName;
		int materialIndex;
		bool textured = false;
		bool hasMetallic = false;
		bool hadRoughness = false;
		bool hasDiffuse = false;
		size_t vertexStartIndex;
		size_t vertexCount;
		
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<glm::vec2> texcoords;
		Buffer vertexBuffer;
		Buffer indexBuffer;
		glm::vec3 color;
		AABB meshAABB;
		Buffer AABB_buffer;
		std::vector<Vertex> aabbVertices;
		Buffer AABB_indexBuffer;
	};

	struct ModelPushConstant
	{
		glm::mat4 model;
		int textureIndex;
		bool isTextured;
	};

	class Model
	{
		public:
			// @filepath - take in the file path of where the model is e.g. "../Desktop/resources/models/sponza.obj
			// @context - vulkan context which houses device
			// @filetype - type of file, fbx or obj
			Model(const std::string& filepath, const VulkanContext& context, int filetype);
			
			// This will draw the the model without debug properties rendered
			void Draw(VkCommandBuffer cmd, VkPipelineLayout layout);

			// This will draw the model will debug prperties visibile such as AABB
			void DrawDebug(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline AABBPipeline);

			// Get the AABB min and max 
			glm::vec3 GetAABBMin() { return m_AABB.min; };
			glm::vec3 GetAABBMax() { return m_AABB.max; };

			std::vector<Material> materials;
			std::vector<Mesh> meshes;
			
			//transformations
			// Note from Ahmad: why are all these transformations public? 
			bool player = false; // Note from Ahmad: why is this here? 
			glm::vec3 translation = glm::vec3(0.f, 0.f, 0.f);
			float rotationX = 0.f;
			float rotationY = 0.f;
			float rotationZ = 0.f;
			glm::mat4 rotMatrix = glm::mat4(1.0f);
			glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);
			bool hasAnimations = false; 
			aiAnimation** animations; 
			std::string modelName;
		private:
			void LoadOBJModel(const std::string& filepath);
			void LoadFBXModel(const std::string& filepath);
			void CreateBuffers();
		private:
			std::vector<Image> loadedTextures;
			std::vector<Image> MetallicTextures;
			std::vector<VkDescriptorSet> m_descriptorSet;
			const VulkanContext& context;
			std::string m_filePath;
			AABB m_AABB;
	};
}

template<>
struct std::hash<Enigma::Vertex>
{
	std::size_t operator()(const Enigma::Vertex& v) const noexcept
	{

		auto h1 = std::hash<glm::vec3>()(v.pos);
		auto h2 = std::hash<glm::vec2>()(v.tex);
		auto h3 = std::hash<glm::vec3>()(v.color);
		return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
	}
};