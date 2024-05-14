#pragma once

#include "Model.h"
#include <memory>
#include "../Core/Camera.h"
#include "Common.h"
#include "../Core/Settings.h"
#include <glm/gtx/euler_angles.hpp>

#include "Physics.h"

class VulkanContext;
class Time;


namespace Enigma
{
	class Player
	{
	public:
		Player(VulkanContext& context, const std::string& model, const glm::vec3 startingPos, int health, Time& time) : m_position{ startingPos }, m_health{ health }
		{
			// this is not the right aspect ratio and needs to change ? but update should handle it 
			FPSCamera = new Camera(m_position, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0, 1, 0), time, 1920.0f / 1080.0f);
			FPSCamera->SetFoV(58.0f);
			m_Model = new Model(model, context, ENIGMA_LOAD_OBJ_FILE);
			m_Model->modelName = "Player";
			m_Model->setTranslation(m_position);
			m_Model->player = true;

			m_AABB = { {-9.0f, -5.0f, -9.0f}, {15.0f, 5.0f, 15.0f} };

			auto minPoint = m_AABB.min;
			auto maxPoint = m_AABB.max;
			aabbVertices.resize(8);

			aabbVertices[0] = Vertex{ {minPoint.x, maxPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right back
			aabbVertices[1] = Vertex{ {maxPoint.x, maxPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right back
			aabbVertices[2] = Vertex{ {maxPoint.x, minPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right front
			aabbVertices[3] = Vertex{ {minPoint.x, minPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right front

			aabbVertices[4] = Vertex{ {minPoint.x, maxPoint.y, maxPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top left back
			aabbVertices[5] = Vertex{ {maxPoint.x, maxPoint.y, maxPoint.z}, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // bottom left back
			aabbVertices[6] = Vertex{ {maxPoint.x, minPoint.y, maxPoint.z}, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // top left front 
			aabbVertices[7] = Vertex{ {minPoint.x, minPoint.y, maxPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom left front
		
			MakeBuffers(context);
		
		}

		void MakeBuffers(VulkanContext& context)
		{

			{
				VkDeviceSize vertexSize = sizeof(aabbVertices[0]) * aabbVertices.size();
				AABB_buffer = CreateBuffer(context.allocator, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

				Buffer stagingBuffer = CreateBuffer(context.allocator, vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);

				void* data = nullptr;
				ENIGMA_VK_CHECK(vmaMapMemory(context.allocator.allocator, stagingBuffer.allocation, &data), "Failed to map staging buffer memory while loading model.");
				std::memcpy(data, aabbVertices.data(), vertexSize);
				vmaUnmapMemory(context.allocator.allocator, stagingBuffer.allocation);

				// Transfering CPU visible data to GPU ( we need a fence to sync ) 
				VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
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
				ENIGMA_VK_CHECK(vkCreateCommandPool(context.device, &cmdPool, nullptr, &commandPool), "Failed to create command pool for staging buffer in model class");

				VkCommandBufferAllocateInfo cmdAlloc{};
				cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				cmdAlloc.commandPool = commandPool;
				cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				cmdAlloc.commandBufferCount = 1;

				VkCommandBuffer cmd = VK_NULL_HANDLE;
				ENIGMA_VK_CHECK(vkAllocateCommandBuffers(context.device, &cmdAlloc, &cmd), "Failed to allocate command buffers while loading model.");

				// begin recording command buffers to do the copying from CPU to GPU buffer
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0;
				beginInfo.pInheritanceInfo = nullptr;

				Enigma::BeginCommandBuffer(cmd);

				// Specify the copy position
				VkBufferCopy copy{};
				copy.size = vertexSize;

				vkCmdCopyBuffer(cmd, stagingBuffer.buffer, AABB_buffer.buffer, 1, &copy);

				VkBufferMemoryBarrier bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
				bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				bufferBarrier.buffer = AABB_buffer.buffer;
				bufferBarrier.size = VK_WHOLE_SIZE;
				bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

				// End the command buffer and submit it
				Enigma::EndAndSubmitCommandBuffer(context, cmd);

				vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
				vkDestroyCommandPool(context.device, commandPool, nullptr);
			}

			VkDeviceSize indexSize = sizeof(indices[0]) * indices.size();
			AABB_indexBuffer = CreateBuffer(context.allocator, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

			// We can't directly add data to GPU memory
			// need to add data to CPU visible memory first and then copy into GPU memory
			Buffer stagingBuffer = CreateBuffer(context.allocator, indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);

			void* data = nullptr;
			ENIGMA_VK_CHECK(vmaMapMemory(context.allocator.allocator, stagingBuffer.allocation, &data), "Failed to map staging buffer memory while loading model.");
			std::memcpy(data, indices.data(), indexSize);
			vmaUnmapMemory(context.allocator.allocator, stagingBuffer.allocation);

			// Transfering CPU visible data to GPU ( we need a fence to sync ) 
			VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
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
			ENIGMA_VK_CHECK(vkCreateCommandPool(context.device, &cmdPool, nullptr, &commandPool), "Failed to create command pool for staging buffer in model class");

			VkCommandBufferAllocateInfo cmdAlloc{};
			cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdAlloc.commandPool = commandPool;
			cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdAlloc.commandBufferCount = 1;

			VkCommandBuffer cmd = VK_NULL_HANDLE;
			ENIGMA_VK_CHECK(vkAllocateCommandBuffers(context.device, &cmdAlloc, &cmd), "Failed to allocate command buffers while loading model.");

			// begin recording command buffers to do the copying from CPU to GPU buffer
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			Enigma::BeginCommandBuffer(cmd);

			// Specify the copy position
			VkBufferCopy copy{};
			copy.size = indexSize;

			vkCmdCopyBuffer(cmd, stagingBuffer.buffer, AABB_indexBuffer.buffer, 1, &copy);

			VkBufferMemoryBarrier bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.buffer = AABB_indexBuffer.buffer;
			bufferBarrier.size = VK_WHOLE_SIZE;
			bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

			// End the command buffer and submit it
			Enigma::EndAndSubmitCommandBuffer(context, cmd);

			vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
			vkDestroyCommandPool(context.device, commandPool, nullptr);
			
		}

		bool isCollidingWithPlayer(std::vector<Model*> worldMeshes)
		{
			for (auto& model : worldMeshes)
			{
				bool isCollideable = model->modelName != "/Light.obj";

				if (isCollideable)
				{
					for (auto& mesh : model->meshes)
					{
						if (!model->player && !model->enemy) {
							AABB box = mesh.meshAABB;
							box.min = box.min * model->getScale() + model->getTranslation();
							box.max = box.max * model->getScale() + model->getTranslation();

							if (Enigma::Physics::isAABBColliding(m_AABB, m_position, box))
							{
								printf("Colliding with: %s\n", mesh.meshName);
								return true;
							}
						}
					}
				}
			}
			return false;
		}

		void Draw(VkCommandBuffer cmd, VkPipelineLayout layout)
		{
			m_Model->Draw(cmd, layout);
		}

		void DrawAABBDebug(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline AABBPipeline, VkDescriptorSet descriptorSet)
		{
			ModelPushConstant push = {};
			push.model = glm::mat4(1.0f);
			push.model = glm::translate(push.model, m_position);
			push.model = glm::scale(push.model, m_Model->getScale());
			push.textureIndex = 0;
			push.isTextured = false;

			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPushConstant), &push);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, AABBPipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &descriptorSet, 0, nullptr);

			VkDeviceSize offset[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &AABB_buffer.buffer, offset);

			vkCmdBindIndexBuffer(cmd, AABB_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}

		~Player()
		{
			delete FPSCamera;
			delete m_Model;
		}

		void TakeDamage(int amount)
		{
			m_health -= amount;
		}

		void setScale(glm::vec3 s) {
			m_Model->setScale(s);
			m_AABB.max = m_AABB.max * m_Model->getScale();
			m_AABB.min = m_AABB.min * m_Model->getScale();
		}

		void Update(GLFWwindow* window, uint32_t width, uint32_t height, const VulkanContext& context, const std::vector<Model*> meshes, const Time& time)
		{
			glm::vec3 cameraPosition = m_position;
			m_position.y = 10.0f;
			FPSCamera->SetPosition(cameraPosition);
			FPSCamera->Update(width, height);
			//glm::vec cameraPosition = FPSCamera->GetPosition();
			//cameraPosition.y = 5.0f;
			//FPSCamera->SetPosition(cameraPosition);

			float rotY = glm::radians(FPSCamera->yaw - 90.0f);
			float rotX = glm::radians(FPSCamera->pitch);

			glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), -rotY, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 pitchRotation = glm::rotate(glm::mat4(1.0f), -rotX, glm::vec3(1.0f, 0.0f, 0.0f));

			glm::mat4 combinedRotation = yawRotation * pitchRotation;

			m_Model->setRotationMatrix(combinedRotation);

			glm::vec3 modelPosition = (FPSCamera->GetPosition() + glm::normalize(FPSCamera->GetDirection())) * 1.f;

			//m_position = modelPosition;
			//m_position.y -= 3.0f;
			//m_Model->setTranslation(m_position);

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
			{
				std::cout << "Shooting" << std::endl;
				float rayLength = 30.0f;
				Enigma::Physics::Ray ray = Enigma::Physics::RayCast(FPSCamera, rayLength);
				glm::vec3 endPoint = ray.origin + ray.direction * rayLength;

				Model* model = new Model("../resources/cube.obj", context, ENIGMA_LOAD_OBJ_FILE);
				model->setTranslation(endPoint);
				Enigma::tempModels.push_back(std::move(model));

				// TODO: OPTIMIZE: if we intersect with the main models mesh then search for which specific mesh of the model we hit
				for (int model = 0; model < meshes.size(); model++)
				{
					for (uint32_t i = 0; i < meshes[model]->meshes.size(); i++)
					{
						AABB box = meshes[model]->meshes[i].meshAABB;
						box.min = box.min * meshes[model]->getScale() + meshes[model]->getTranslation();
						box.max = box.max * meshes[model]->getScale() + meshes[model]->getTranslation();
						if (Enigma::Physics::RayIntersectAABB(ray, box) && meshes[model]->enemy)
						{
							// could clear info regarding the mesh which got hit instead?
							std::cout << "Hit: " << meshes[model]->meshes[i].meshName << std::endl;
							meshes[model]->hit = true;
							break;
						}
					}
				}

			}

			glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);

			// Set the speed if the player is sprinting 
			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			{
				speed = 30.0f;
			}
			else {
				speed = 20.0f;
			}

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			{
				glm::mat4 invView = glm::inverse(FPSCamera->GetCameraTransform().view);
				velocity += -glm::vec3(invView[2]);

				velocity = glm::normalize(velocity) * speed * (float)time.deltaTime; // need to multiply by delta time 
			}

			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			{
				glm::mat4 invView = glm::inverse(FPSCamera->GetCameraTransform().view);
				velocity -= -glm::vec3(invView[2]);

				velocity = glm::normalize(velocity) * speed * (float)time.deltaTime; // need to multiply by delta time 
			}

			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			{
				glm::mat4 invView = glm::inverse(FPSCamera->GetCameraTransform().view);
				auto cameraFront = -glm::vec3(invView[2]);
				auto cameraUp = FPSCamera->GetUp();

				auto right = glm::normalize(glm::cross(cameraFront, cameraUp));

				velocity += right;
				velocity = glm::normalize(velocity) * speed * (float)time.deltaTime; // need to multiply by delta time 
			}

			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			{
				glm::mat4 invView = glm::inverse(FPSCamera->GetCameraTransform().view);
				auto cameraFront = -glm::vec3(invView[2]);
				auto cameraUp = FPSCamera->GetUp();

				auto right = glm::normalize(glm::cross(cameraFront, cameraUp));

				velocity -= right;

				velocity = glm::normalize(velocity) * speed * (float)time.deltaTime; // need to multiply by delta time 
			}

			m_position += velocity;
			if (isCollidingWithPlayer(meshes))
			{
				m_position -= velocity;
			}
			else {
				cameraPosition = m_position + glm::vec3(0.0f, 5.0f, 0.0f);
				FPSCamera->SetPosition(cameraPosition);

				modelPosition.y -= 1.0f;
				m_Model->setTranslation(modelPosition);
			}
		}

		void SetPosition(glm::vec3 newpos)
		{
			m_position = newpos;
		}

		static void PlayerKeyCallback(GLFWwindow* window, int key, int, int action, int)
		{
			if (GLFW_KEY_C == key && action == GLFW_PRESS)
			{
				Enigma::isDebug = true;
				Enigma::enablePlayerCamera = false;
			}
		}

		Camera* GetCamera() const { return FPSCamera; }

		glm::vec3 GetPosition() const { return m_position; }
		AABB GetAABB() const { return m_AABB; }
		int GetHealth() const { return m_health; }
		Model* m_Model;
		int navmeshPosition;

	private:
		Camera* FPSCamera;
		glm::vec3 m_position;
		glm::vec3 m_direction;
		int m_health = 0;
		AABB m_AABB;
		float speed = 1.0f;

		Buffer AABB_buffer;
		std::vector<Vertex> aabbVertices;
		Buffer AABB_indexBuffer;

		std::vector<uint32_t> indices = {
			0,1,
			1,2,
			2,3,
			3,0,
			0,4,
			4,5,
			5,1,
			5,6,
			6,2,
			6,7,
			7,4,
			7,3
		};
	};
}

// you have player position and direction
// player position is the camera direction 