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

#include <cmath>

#define ENIGMA_LOAD_OBJ_FILE 0
#define ENIGMA_LOAD_FBX_FILE 1
#define ENIGMA_LOAD_ASSIMP_FILE 2

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
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
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
	/*
	struct BoneInfo
	{
		aiMatrix4x4 offset;
		aiMatrix4x4 finalTransformation;

		BoneInfo() : offset(), finalTransformation() {}
	};
	*/

	struct KeyPosition {
		float time;
		glm::vec3 position;
	};

	struct KeyRotation {
		float time;
		glm::quat rotation;
	};

	struct KeyScale {
		float time;
		glm::vec3 scale;
	};

	struct Node {
		std::string name;
        Node* parent{};
		std::vector<std::shared_ptr<Node>> children;

        glm::vec3 translation{};
        glm::vec3 scale{1,1,1};
        glm::quat rotation{1,0,0,0};

        glm::mat4 localMatrix=glm::mat4(1);
        glm::mat4 globalMatrix=glm::mat4(1);

        glm::mat4 boneOffsetMatrix=glm::mat4(1);

        std::vector<int> meshIndices;

        int index = 0; //for animation

        void Update(bool updateChildren = true) {
            localMatrix = glm::scale(glm::translate(glm::mat4(1), translation) * glm::mat4_cast(rotation), scale);

            auto m = glm::mat4(1);
            if (parent != nullptr) {
                m = parent->globalMatrix;
            }
            globalMatrix= m * localMatrix;
            if (updateChildren)
                for (auto&& e : children) e->Update();
        }
	};

	struct BoneInfo {
		glm::mat4 offsetMatrix;
		glm::mat4 finalTransformation;
		
	};

	struct NodeAnim {
		std::string nodeName;
        Node* node;
		std::vector<KeyPosition> positions;
		std::vector<KeyRotation> rotations;
		std::vector<KeyScale> scales;
	};
	struct Animation {
		float ticksPerSecond;
		float duration;
		std::vector<NodeAnim> channel; // Channel of animation for each node
	};

	class Model
	{
		public:
			Model(const std::string& filepath, const VulkanContext& context, int filetype);
			void Draw(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline aabPipeline);
			void Draw2(VkCommandBuffer cmd, VkPipelineLayout layout);
			void DrawAABB(VkCommandBuffer cmd, VkPipelineLayout layout);

			glm::vec3 GetAABBMin() { return m_AABB.min; };
			glm::vec3 GetAABBMax() { return m_AABB.max; };
			
			std::vector<Material> materials;
			std::vector<Mesh> meshes;
			
			bool player = false;
			bool equipment = false;
			bool hasAnimations = false;
			const aiScene* m_Scene;

			std::unordered_map<std::string, int> boneMapping; // Global bone index mapping

			//2021/04/29
			std::vector<Animation> animations2;
			Node rootNode;
			std::vector<BoneInfo> boneInfo;
			glm::mat4 globalInverseTransform;
			unsigned int numBones;

			void updateAnimation(float deltaTime,int index=0);
			//void readNodeHierarchy(float animationTime, Node* node, const glm::mat4& parentTransform, const Animation& anim);

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
			
			const NodeAnim* findNodeAnim(const Animation& animation, const std::string& nodeName) {
				for (const auto& channel : animation.channel) {
					if (channel.nodeName == nodeName) {
						return &channel;
					}
				}
				return nullptr;
			}

			// Returns a 4x4 matrix that inserts the conversion between the current frame and the next frame
			glm::vec3 interpolateTranslation(float time, const NodeAnim* pNodeAnim) {
				glm::vec3 translation;

				if (pNodeAnim->positions.size() == 1) {
					translation = pNodeAnim->positions[0].position;
				}
				else {
					size_t frameIndex = 0;
					for (size_t i = 0; i < pNodeAnim->positions.size() - 1; i++) {
						if (time < pNodeAnim->positions[i + 1].time) {
							frameIndex = i;
							break;
						}
					}

					auto& currentFrame = pNodeAnim->positions[frameIndex];
					auto& nextFrame = pNodeAnim->positions[(frameIndex + 1) % pNodeAnim->positions.size()];
					float delta = (time - currentFrame.time) / (nextFrame.time - currentFrame.time);
					translation = glm::mix(currentFrame.position, nextFrame.position, delta);
				}

				return translation;
			}

			//Returns a 4x4 matrix with a rotation inserted between the current frame and the next
			glm::quat interpolateRotation(float time, const NodeAnim* pNodeAnim) {
				glm::quat rotation;

				if (pNodeAnim->rotations.size() == 1) {
					rotation = pNodeAnim->rotations[0].rotation;
				}
				else {
					size_t frameIndex = 0;
					for (size_t i = 0; i < pNodeAnim->rotations.size() - 1; i++) {
						if (time < pNodeAnim->rotations[i + 1].time) {
							frameIndex = i;
							break;
						}
					}

					auto& currentFrame = pNodeAnim->rotations[frameIndex];
					auto& nextFrame = pNodeAnim->rotations[(frameIndex + 1) % pNodeAnim->rotations.size()];
					float delta = (time - currentFrame.time) / (nextFrame.time - currentFrame.time);
					rotation = glm::slerp(currentFrame.rotation, nextFrame.rotation, delta);
				}
                return rotation;
			}

			// Returns a 4x4 matrix with a zoom inserted between the current frame and the next
			glm::vec3 interpolateScale(float time, const NodeAnim* pNodeAnim) {
				glm::vec3 scale;

				if (pNodeAnim->scales.size() == 1) {
					scale = pNodeAnim->scales[0].scale;
				}
				else {
					size_t frameIndex = 0;
					for (size_t i = 0; i < pNodeAnim->scales.size() - 1; i++) {
						if (time < pNodeAnim->scales[i + 1].time) {
							frameIndex = i;
							break;
						}
					}

					const KeyScale& currentScale = pNodeAnim->scales[frameIndex];
					const KeyScale& nextScale = pNodeAnim->scales[(frameIndex + 1) % pNodeAnim->scales.size()];
					float delta = (time - currentScale.time) / (nextScale.time - currentScale.time);
					scale = glm::mix(currentScale.scale, nextScale.scale, delta);
				}

				return scale;
			}
		public:
			void setTranslation(glm::vec3 trans) { translation = trans; 
            rootNode.translation=trans;
            };
			void setScale(glm::vec3 s) { scale = s; 
            rootNode.scale=s;
            };
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

			//aiMatrix4x4 globalInverseTransform;

		private:
			void LoadOBJModel(const std::string& filepath);
			void LoadFBXModel(const std::string& filepath);
			void LoadModelAssimp(const std::string& filepath);
			void CreateBuffers();

			//std::vector<BoneInfo> boneInfo;
			//uint32_t numBones = 0;
			// Bone transformations
			std::vector<glm::mat4> boneTransforms;
            Buffer boneTransformBuffer;
			std::vector<VkDescriptorSet> boneTransformDescriptorSet;

            //===========================
            std::unordered_map<std::string, Node*> tempNodeMap;
			void processNode(aiNode* node, Node* dstNode, int& index);
			void processMesh(aiMesh* mesh);
            void processAnimation();
            void loadBones();
            void loadMaterials();
            void createBoneTransformBuffer();
            void drawNode(VkCommandBuffer cmd,VkPipelineLayout layout,Node* node);
            void drawNodeAABB(VkCommandBuffer cmd,VkPipelineLayout layout,Node* node);
            void updateBoneTransforms();
			void updateBoneTransformsHelper(Node* node);
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