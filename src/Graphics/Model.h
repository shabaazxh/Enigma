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

#define MAX_BONES 64
#define MAX_BONES_PER_VERTEX 4

namespace Enigma
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 tex;
		glm::vec3 color;

		// Arrays to store bone IDs and weights for skeletal animation
		std::array<uint32_t, MAX_BONES_PER_VERTEX> boneIDs = { 0, 0, 0, 0 };
		std::array<float, MAX_BONES_PER_VERTEX> weights = { 0.f, 0.f, 0.f, 0.f };

		// Method to add bone data to a vertex
		void add(uint32_t boneID, float weight) {
			for (uint32_t i = 0; i < MAX_BONES_PER_VERTEX; i++) {
				if (weights[i] == 0.0f) {
					boneIDs[i] = boneID;
					weights[i] = weight;
					return;
				}
			}
		}

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

			// Bone IDs
			attributes[4].binding = 0;
			attributes[4].location = 3;
			attributes[4].format = VK_FORMAT_R32G32B32A32_SINT; // Use SINT for bone IDs
			attributes[4].offset = offsetof(Vertex, boneIDs);

			// Bone weights
			attributes[5].binding = 0;
			attributes[5].location = 4;
			attributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT; // Use SFLOAT for weights
			attributes[5].offset = offsetof(Vertex, weights);

			return attributes;
			
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
		Buffer vertexBuffer;
		Buffer indexBuffer;
		AABB meshAABB;
		Buffer AABB_buffer;
		std::vector<Vertex> aabbVertices;
		Buffer AABB_indexBuffer;
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
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
			Model(const std::string& filepath, const VulkanContext& context, int filetype, const std::string& name);
			
			// This will draw the the model without debug properties rendered
			void Draw(VkCommandBuffer cmd, VkPipelineLayout layout);

			// This will draw the model will debug prperties visibile such as AABB
			void DrawDebug(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline AABBPipeline);
			void DrawDebug(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline AABBPipeline, int index);

			// Get the AABB min and max 
			glm::vec3 GetAABBMin() const { return m_AABB.min; };
			glm::vec3 GetAABBMax() const { return m_AABB.max; };

			std::vector<Material> materials;
			std::vector<Mesh> meshes;

			bool player = false;
			bool enemy = false;
			bool hasAnimations = false;
			bool hit = false;
			aiAnimation** animations;

			std::string modelName;
			glm::mat4 modelMatrix = glm::mat4(1.0f);
			std::vector<VkDescriptorSet> m_descriptorSet;

		private:
			glm::vec3 translation = glm::vec3(0.f, 0.f, 0.f);
			float rotationX = 0.f;
			float rotationY = 0.f;
			float rotationZ = 0.f;
			glm::mat4 rotMatrix = glm::mat4(1.0f);
			glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);
			glm::vec3 offset = glm::vec3(0.f, 0.f, 0.f);
			std::vector<Image> loadedTextures;
			std::vector<Image> MetallicTextures;
			
			const VulkanContext& context;
			std::string m_filePath;
			AABB m_AABB;

			std::unordered_map<std::string, int> boneMapping;
			unsigned int numBones;

		public:
			void setTranslation(glm::vec3 trans) { translation = trans; };
			void setScale(glm::vec3 s) { scale = s; };
			void setRotationX(float angle) { rotationX = angle; };
			void setRotationY(float angle) { rotationY = angle; };
			void setRotationZ(float angle) { rotationZ = angle; };
			void setRotationMatrix(glm::mat4 rm) { rotMatrix = rm; };
			void setOffset(glm::vec3 v) { offset = v; }
			glm::vec3 getTranslation() { return translation; }
			float getXRotation() { return rotationX; }
			float getYRotation() { return rotationY; }
			float getZRotation() { return rotationZ; }
			glm::mat4 getRotationMatrix() { return rotMatrix; }
			glm::vec3 getScale() { return scale; }

		private:
			void LoadOBJModel(const std::string& filepath);
			void LoadFBXModel(const std::string& filepath);
			void CreateBuffers();
			void CreateAABBBuffers();

			void loadBones(aiMesh* mesh, std::vector<Vertex>& boneData);
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