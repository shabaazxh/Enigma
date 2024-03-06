#include "Model.h"
#include "VulkanObjects.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#include <rapidobj.hpp>
#include <unordered_set>
#include "../Graphics/Common.h"

namespace Enigma
{
	Model::Model(const std::string& filepath, Allocator& aAllocator, const VulkanContext& context) : m_filePath{filepath}, allocator{ aAllocator }, context{context}
	{
		LoadModel(filepath);
		GetBoundingBox();
	}

	void Model::LoadModel(const std::string& filepath)
	{
		// Load the obj file
		rapidobj::Result result = rapidobj::ParseFile(filepath.c_str());
		if (result.error) {
			ENIGMA_ERROR("Failed to load model.");
			throw std::runtime_error("Failed to load model");
		}
		
		// obj can have non triangle faces. Triangulate will triangulate
		// non triangle faces
		rapidobj::Triangulate(result);


		// store the prefix to the obj file
		const char* pathBegin = filepath.c_str();
		const char* pathEnd = std::strrchr(pathBegin, '/');

		const std::string prefix = pathEnd ? std::string(pathBegin, pathEnd + 1) : "";

		m_filePath = filepath;

		// Find the materials for this model (searching for diffuse only at the moement)
		for (const auto& mat : result.materials)
		{
			Material mi;
			mi.materialName = mat.name;
			mi.diffuseColour = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
			
			if (!mat.diffuse_texname.empty())
				mi.diffuseTexturePath = prefix + mat.diffuse_texname;

			materials.emplace_back(std::move(mi));
		}

		// Extract mesh data 
		std::unordered_set<size_t> activeMaterials;
		for (const auto& shape : result.shapes)
		{
			const auto& shapeName = shape.name;

			activeMaterials.clear();

			for (size_t i = 0; i < shape.mesh.indices.size(); i++)
			{
				const auto faceID = i / 3;
				assert(faceID < shape.mesh.material_ids.size());

				const auto matID = shape.mesh.material_ids[faceID];
				assert(matID < int(materials.size()));
				activeMaterials.emplace(matID);
			}
;
			// process vertices for materials which are active
			for (const auto matID : activeMaterials)
			{
				Vertex vertex{};
				auto* pos = &vertex.pos;
				auto* tex = &vertex.tex;

				const bool textured = !materials[matID].diffuseTexturePath.empty();
				
				if (!textured)
				{
					tex = nullptr;
				}

				std::string meshName;
				if (activeMaterials.size() == 1)
					meshName = shapeName;
				else
					meshName = shapeName + "::" + materials[matID].materialName;

				// ==== THIS SEEMS LIKE A POINTLESS CHECK?
				// Extract material's vertices
				// const auto firstVertex = pos->size();
				// assert(!textured || firstVertex == tex->size());


				Mesh mesh{};
				for (size_t i = 0; i < shape.mesh.indices.size(); i++)
				{
					const auto faceID = i / 3;
					const auto faceMat = size_t(shape.mesh.material_ids[faceID]);

					if (faceMat != matID)
						continue;

					const auto& idx = shape.mesh.indices[i];

					vertex.pos = (glm::vec3{
						result.attributes.positions[idx.position_index * 3 + 0],
						result.attributes.positions[idx.position_index * 3 + 1],
						result.attributes.positions[idx.position_index * 3 + 2]
					});

					if (textured)
					{
						vertex.tex = (glm::vec2(
							result.attributes.texcoords[idx.texcoord_index * 2 + 0],
							result.attributes.texcoords[idx.texcoord_index * 2 + 1]
						));
					}

					vertex.color = glm::vec3(0.4f, 0.3f, 1.0f);

					mesh.vertices.emplace_back(vertex);
				}

				// const auto vertexCount = pos->size() - firstVertex;
				// assert(!textured || vertexCount == tex->size() - firstVertex);

				mesh.materialIndex = matID;
				mesh.meshName = std::move(meshName);
				mesh.textured = textured;
				meshes.emplace_back(std::move(mesh));
			}
			
		}

		CreateBuffers();
		// need to store it at mesh index not material index when pushing into loaded exxtures
		// C:/Users/Shahb/source/repos/Enigma/Enigma/resources/textures/jpeg/lion.jpg

		const std::string defaultTexture = "C:/Users/Billy/Documents/Enigma/resources/textures/jpeg/lion.jpg";
		Image defaultTex = Enigma::CreateTexture(context, defaultTexture, allocator);

		loadedTextures.resize(materials.size());
		for (int i = 0; i < materials.size(); i++)
		{
			if (materials[i].diffuseTexturePath != "")
			{
				Image texture = Enigma::CreateTexture(context, materials[i].diffuseTexturePath, allocator);
				loadedTextures[i] = std::move(texture);
			}
			else
			{
				Image defaultTex = Enigma::CreateTexture(context, defaultTexture, allocator);
				loadedTextures[i] = std::move(defaultTex);
			}
		}

		Enigma::AllocateDescriptorSets(context, Enigma::descriptorPool, Enigma::descriptorLayoutModel, 1, m_descriptorSet);

		std::vector<VkDescriptorImageInfo> imageinfos;

		for (size_t i = 0; i < loadedTextures.size(); i++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = loadedTextures[i].imageView;
			imageInfo.sampler = Enigma::sampler.handle;
					
			imageinfos.emplace_back(std::move(imageInfo));
			
		}
		// 12
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet[0];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(imageinfos.size());
		descriptorWrite.pImageInfo = imageinfos.data();

		vkUpdateDescriptorSets(context.device, 1, &descriptorWrite, 0, nullptr);
	}

	void Model::CreateBuffers()
	{
		for (auto& mesh : meshes)
		{
			VkDeviceSize vertexSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();
			//VkDeviceSize indexSize = sizeof(m_indices[0] * m_indices.size());

			mesh.vertexBuffer = CreateBuffer(allocator, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
			//mesh.indexBuffer = CreateBuffer(allocator, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

			// We can't directly add data to GPU memory
			// need to add data to CPU visible memory first and then copy into GPU memory
			Buffer stagingBuffer = CreateBuffer(allocator, vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);

			void* data = nullptr;
			ENIGMA_VK_ERROR(vmaMapMemory(allocator.allocator, stagingBuffer.allocation, &data), "Failed to map staging buffer memory while loading model.");
			std::memcpy(data, mesh.vertices.data(), vertexSize);
			vmaUnmapMemory(allocator.allocator, stagingBuffer.allocation);

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

			vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &copy);

			VkBufferMemoryBarrier bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.buffer = mesh.vertexBuffer.buffer;
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
	}

	// Call to draw the model
	void Model::Draw(VkCommandBuffer cmd, VkPipelineLayout layout)
	{
		ModelPushConstant push = {};
		push.model = glm::mat4(1.0f);
		push.model = glm::translate(push.model, this->translation);
		push.model = glm::rotate(push.model, glm::radians(this->rotationX), glm::vec3(1, 0, 0));
		push.model = glm::rotate(push.model, glm::radians(this->rotationY), glm::vec3(0, 1, 0));
		push.model = glm::rotate(push.model, glm::radians(this->rotationZ), glm::vec3(0, 0, 1));
		push.model = glm::scale(push.model, this->scale);
		for (auto& mesh : meshes)
		{
			push.textureIndex = mesh.materialIndex;

			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPushConstant), &push);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_descriptorSet[0], 0, nullptr);

			VkDeviceSize offset[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, offset);

			vkCmdDraw(cmd, static_cast<uint32_t>(mesh.vertices.size()), 1, 0, 0);
		}
	}

	void Model::GetBoundingBox() {
		for (int i = 0; i < this->meshes.size(); i++) {
			glm::vec3 minXYZ = this->meshes.at(i).vertices.at(0).pos;
			glm::vec3 maxXYZ = this->meshes.at(i).vertices.at(0).pos;
			for (int j = 1; j < this->meshes.at(i).vertices.size(); j++) {
				if (minXYZ.x > this->meshes.at(i).vertices.at(j).pos.x) {
					minXYZ.x = this->meshes.at(i).vertices.at(j).pos.x;
				}
				else if (maxXYZ.x < this->meshes.at(i).vertices.at(j).pos.x) {
					maxXYZ.x = this->meshes.at(i).vertices.at(j).pos.x;
				}

				if (minXYZ.y > this->meshes.at(i).vertices.at(j).pos.y) {
					minXYZ.y = this->meshes.at(i).vertices.at(j).pos.y;
				}
				else if (maxXYZ.y < this->meshes.at(i).vertices.at(j).pos.y) {
					maxXYZ.y = this->meshes.at(i).vertices.at(j).pos.y;
				}

				if (minXYZ.z > this->meshes.at(i).vertices.at(j).pos.z) {
					minXYZ.z = this->meshes.at(i).vertices.at(j).pos.z;
				}
				else if (maxXYZ.z < this->meshes.at(i).vertices.at(j).pos.z) {
					maxXYZ.z = this->meshes.at(i).vertices.at(j).pos.z;
				}
			}
			this->meshes.at(i).bounding_box.push_back(glm::vec3(minXYZ.x, minXYZ.y, minXYZ.z));
			this->meshes.at(i).bounding_box.push_back(glm::vec3(maxXYZ.x, minXYZ.y, minXYZ.z));
			this->meshes.at(i).bounding_box.push_back(glm::vec3(minXYZ.x, maxXYZ.y, minXYZ.z));
			this->meshes.at(i).bounding_box.push_back(glm::vec3(minXYZ.x, minXYZ.y, maxXYZ.z));
			this->meshes.at(i).bounding_box.push_back(glm::vec3(maxXYZ.x, maxXYZ.y, minXYZ.z));
			this->meshes.at(i).bounding_box.push_back(glm::vec3(minXYZ.x, maxXYZ.y, maxXYZ.z));
			this->meshes.at(i).bounding_box.push_back(glm::vec3(maxXYZ.x, minXYZ.y, maxXYZ.z));
			this->meshes.at(i).bounding_box.push_back(glm::vec3(maxXYZ.x, maxXYZ.y, maxXYZ.z));
		}
	}
}

