#pragma once

#include <Volk/volk.h>
#define GLM_ENABLE_EXPERIMENTAL
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
		glm::vec2 tex;
		glm::vec3 color;
		glm::vec3 normal;

		std::array<uint32_t, MAX_BONES_PER_VERTEX> boneIDs = { 0, 0, 0, 0 };
		std::array<float, MAX_BONES_PER_VERTEX> weights = { 0.f, 0.f, 0.f, 0.f };

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

		static std::array<VkVertexInputAttributeDescription, 5> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 5> attributes{};

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
			
			// Bone IDs
			attributes[3].binding = 0;
			attributes[3].location = 3;
			attributes[3].format = VK_FORMAT_R32G32B32A32_SINT; // Use SINT for bone IDs
			attributes[3].offset = offsetof(Vertex, boneIDs);

			// Bone weights
			attributes[4].binding = 0;
			attributes[4].location = 4;
			attributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT; // Use SFLOAT for weights
			attributes[4].offset = offsetof(Vertex, weights);

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
	};
	
	struct Mesh
	{
		std::string meshName;
		int materialIndex;
		bool textured;
		//bool textured = false;  
		size_t vertexStartIndex;
		size_t vertexCount;
		
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		AABB meshAABB;
		std::vector<Vertex> aabbVertices;
		Buffer vertexBuffer;
		Buffer indexBuffer;
		Buffer AABB_buffer;
		Buffer AABB_indexBuffer;
	};

	struct ModelPushConstant
	{
		glm::mat4 model;
		int textureIndex;
		bool isTextured;
	};

	struct BoneInfo
	{
		aiMatrix4x4 offset;
		aiMatrix4x4 finalTransformation;

		BoneInfo() : offset(), finalTransformation() {}
	};

	class Model
	{
		public:
			Model(const std::string& filepath, const VulkanContext& context, int filetype);
			void Draw(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline aabPipeline);
			glm::vec3 GetAABBMin() { return m_AABB.min; };
			glm::vec3 GetAABBMax() { return m_AABB.max; };
			void updateAnimation(const aiScene* scene, float deltaTime);

			std::vector<Material> materials;
			std::vector<Mesh> meshes;
			
			bool player = false;
			bool equipment = false;
			bool hasAnimations = false;
			const aiScene* m_Scene;

			std::unordered_map<std::string, int> boneMapping; // Global bone index mapping

		private:
			glm::vec3 translation = glm::vec3(0.f, 0.f, 0.f);
			float rotationX = 0.f;
			float rotationY = 0.f;
			float rotationZ = 0.f;
			glm::mat4 rotMatrix = glm::mat4(1.0f);
			glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);
			glm::vec3 offset = glm::vec3(0.f, 0.f, 0.f);
			std::vector<Image> loadedTextures;
			std::vector<VkDescriptorSet> m_descriptorSet;
			const VulkanContext& context;
			std::string m_filePath;
			AABB m_AABB;

			const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string& nodeName) {
				for (uint32_t i = 0; i < animation->mNumChannels; i++) {
					const aiNodeAnim* nodeAnim = animation->mChannels[i];
					if (std::string(nodeAnim->mNodeName.data) == nodeName) {
						return nodeAnim;
					}
				}
				return nullptr;
			}

			// Returns a 4x4 matrix that inserts the conversion between the current frame and the next frame
			aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim* pNodeAnim)
			{
				aiVector3D translation;

				if (pNodeAnim->mNumPositionKeys == 1)
				{
					translation = pNodeAnim->mPositionKeys[0].mValue;
				}
				else
				{
					uint32_t frameIndex = 0;
					for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
					{
						if (time < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
						{
							frameIndex = i;
							break;
						}
					}

					aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
					aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

					float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

					const aiVector3D& start = currentFrame.mValue;
					const aiVector3D& end = nextFrame.mValue;

					translation = (start + delta * (end - start));
				}

				aiMatrix4x4 mat;
				aiMatrix4x4::Translation(translation, mat);
				return mat;
			}

			//Returns a 4x4 matrix with a rotation inserted between the current frame and the next
			aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim* pNodeAnim)
			{
				aiQuaternion rotation;

				if (pNodeAnim->mNumRotationKeys == 1)
				{
					rotation = pNodeAnim->mRotationKeys[0].mValue;
				}
				else
				{
					uint32_t frameIndex = 0;
					for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
					{
						if (time < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
						{
							frameIndex = i;
							break;
						}
					}

					aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
					aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

					float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

					const aiQuaternion& start = currentFrame.mValue;
					const aiQuaternion& end = nextFrame.mValue;

					aiQuaternion::Interpolate(rotation, start, end, delta);
					rotation.Normalize();
				}

				aiMatrix4x4 mat(rotation.GetMatrix());
				return mat;
			}

			// Returns a 4x4 matrix with a zoom inserted between the current frame and the next
			aiMatrix4x4 interpolateScale(float time, const aiNodeAnim* pNodeAnim)
			{
				aiVector3D scale;

				if (pNodeAnim->mNumScalingKeys == 1)
				{
					scale = pNodeAnim->mScalingKeys[0].mValue;
				}
				else
				{
					uint32_t frameIndex = 0;
					for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
					{
						if (time < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
						{
							frameIndex = i;
							break;
						}
					}

					aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
					aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

					float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

					const aiVector3D& start = currentFrame.mValue;
					const aiVector3D& end = nextFrame.mValue;

					scale = (start + delta * (end - start));
				}

				aiMatrix4x4 mat;
				aiMatrix4x4::Scaling(scale, mat);
				return mat;
			}

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

			aiMatrix4x4 globalInverseTransform;

		private:
			void LoadOBJModel(const std::string& filepath);
			void LoadFBXModel(const std::string& filepath);
			void CreateBuffers();

			std::vector<BoneInfo> boneInfo;
			uint32_t numBones = 0;
			// Bone transformations
			std::vector<aiMatrix4x4> boneTransforms;

			void loadBones(aiMesh* mesh, std::vector<Vertex>& boneData);

			void processNode(aiNode* node, const aiScene* scene);
			void processMesh(aiMesh* mesh, const aiScene* scene);
			//void LoadAnimations(const aiScene* scene);

			void readNodeHierarchy(const aiNode* node, const aiMatrix4x4& parentTransform, float animationTime, const aiAnimation* anim);

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