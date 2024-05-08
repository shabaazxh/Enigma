#include "Model.h"
#include "VulkanObjects.h"
#include <rapidobj.hpp>
#include <unordered_set>
#include "../Graphics/Common.h"
#include "../Core/Engine.h"
#include <unordered_map>
#include <string>

namespace Enigma
{

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

	Model::Model(const std::string& filepath, const VulkanContext& context, int filetype) : m_filePath{filepath}, context{context}
	{
		if (filetype == ENIGMA_LOAD_OBJ_FILE)
			LoadOBJModel(filepath);
		else if (filetype == ENIGMA_LOAD_FBX_FILE)
			LoadFBXModel(filepath);
        else
            LoadModelAssimp(filepath);
    }

	void Model::LoadOBJModel(const std::string& filepath)
	{
		// Load the obj file
		rapidobj::Result result = rapidobj::ParseFile(filepath.c_str());
		if (result.error) {
			ENIGMA_ERROR("Failed to load model.");
			throw std::runtime_error("Failed to load model");
		}

		for (const auto& shape : result.shapes) {
			//finds if the model has a navmesh
			if (shape.name == "Navmesh") {
				std::vector<int> vertices;
				//loop through the edges of navmesh adding unique vertices to the navmesh structure
				for (int i = 0; i < shape.lines.indices.size(); i++) {
					bool inVec = false;
					for (int j = 0; j < vertices.size(); j++) {
						if (shape.lines.indices[i].position_index == vertices[j]) {
							inVec = true;
							break;
						}
					}
					if (inVec == false) {
						vertices.push_back(shape.lines.indices[i].position_index);
						glm::vec3 tempvec = glm::vec3(result.attributes.positions[(shape.lines.indices[i].position_index * 3)], result.attributes.positions[(shape.lines.indices[i].position_index * 3) + 1], result.attributes.positions[(shape.lines.indices[i].position_index * 3) + 2]);
						navmesh.vertices.push_back(tempvec);
					}
				}
				navmesh.edges.resize(navmesh.vertices.size());
				//loop through the edges adding the index of each neighbouring vertex to each vertex
				for (int i = 0; i < shape.lines.indices.size(); i+=2) {
					for (int j = 0; j < navmesh.vertices.size(); j++) {
						if (result.attributes.positions[(shape.lines.indices[i].position_index * 3)] == navmesh.vertices[j].x &&
							result.attributes.positions[(shape.lines.indices[i].position_index * 3) + 1] == navmesh.vertices[j].y &&
							result.attributes.positions[(shape.lines.indices[i].position_index * 3) + 2] == navmesh.vertices[j].z) {
								Edge edge;
								glm::vec3 thisVert = navmesh.vertices[j];
								glm::vec3 otherVert = glm::vec3(result.attributes.positions[(shape.lines.indices[i+1].position_index * 3)], result.attributes.positions[(shape.lines.indices[i+1].position_index * 3) + 1], result.attributes.positions[(shape.lines.indices[i+1].position_index * 3) + 2]);
								glm::vec3 edgeDir = thisVert - otherVert;
								float weight = vec3Length(edgeDir);
								int otherVertIndex;
								for (int k = 0; k < navmesh.vertices.size(); k++) {
									if (navmesh.vertices[k].x == otherVert.x &&
										navmesh.vertices[k].y == otherVert.y &&
										navmesh.vertices[k].z == otherVert.z) {
											Edge edge2;
											otherVertIndex = k;
											edge2.vertex2 = j;
											edge2.weight = weight;
											navmesh.edges[k].push_back(edge2);
											break;
									}
								}
								edge.vertex2 = otherVertIndex;
								edge.weight = weight;

								navmesh.edges[j].push_back(edge);
								break;
						}
					}
				}
				navmesh.numberOfBaseNodes = navmesh.vertices.size();
			}
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
				const bool textured = !materials[matID].diffuseTexturePath.empty();

				if (!textured)
				{
					vertex.color = materials[matID].diffuseColour;
				}

				std::string meshName;
				if (activeMaterials.size() == 1)
					meshName = shapeName;
				else
					meshName = shapeName + "::" + materials[matID].materialName;

				Mesh mesh{};
				std::unordered_map<Vertex, uint32_t> uniqueVertices;
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

					vertex.normal = (glm::vec3{
						result.attributes.normals[idx.position_index * 3 + 0],
						result.attributes.normals[idx.position_index * 3 + 1],
						result.attributes.normals[idx.position_index * 3 + 2]
						});

					if (textured)
					{
						vertex.tex = (glm::vec2(
							result.attributes.texcoords[idx.texcoord_index * 2 + 0],
							result.attributes.texcoords[idx.texcoord_index * 2 + 1]
						));
					}

					if (uniqueVertices.count(vertex) == 0)
					{
						uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
						mesh.vertices.push_back(vertex);
					}

					mesh.indices.push_back(uniqueVertices[vertex]);
				}

				mesh.materialIndex = (int)matID;
				mesh.meshName = std::move(meshName);
				mesh.textured = textured;
				meshes.emplace_back(std::move(mesh));
			}

		}

		const std::string defaultTexture = "../resources/textures/jpeg/sponza_floor_a_diff.jpg";
		loadedTextures.resize(materials.size());
		for (int i = 0; i < materials.size(); i++)
		{
			if (materials[i].diffuseTexturePath != "")
			{
				Image texture = Enigma::CreateTexture(context, materials[i].diffuseTexturePath);
				loadedTextures[i] = std::move(texture);
			}
			else
			{
				Image defaultTex = Enigma::CreateTexture(context, defaultTexture);
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
			imageInfo.sampler = Enigma::defaultSampler;

			imageinfos.emplace_back(std::move(imageInfo));

		}
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet[0];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(imageinfos.size());
		descriptorWrite.pImageInfo = imageinfos.data();

		vkUpdateDescriptorSets(context.device, 1, &descriptorWrite, 0, nullptr);
	
		
		for (auto& mesh : meshes)
		{
			std::vector<Vertex> verts;
			verts.insert(verts.begin(), mesh.vertices.begin(), mesh.vertices.end());

			float minX = std::numeric_limits<float>::max();
			float minY = std::numeric_limits<float>::max();
			float minZ = std::numeric_limits<float>::max();

			float maxX = -std::numeric_limits<float>::max();
			float maxY = -std::numeric_limits<float>::max();
			float maxZ = -std::numeric_limits<float>::max();

			for (unsigned int i = 0; i < verts.size(); i++)
			{
				minX = std::min(minX, verts[i].pos.x);
				minY = std::min(minY, verts[i].pos.y);
				minZ = std::min(minZ, verts[i].pos.z);
														
				maxX = std::max(maxX, verts[i].pos.x);
				maxY = std::max(maxY, verts[i].pos.y);
				maxZ = std::max(maxZ, verts[i].pos.z);
			}

			glm::vec3 minPoint = glm::vec3(minX, minY, minZ);
			glm::vec3 maxPoint = glm::vec3(maxX, maxY, maxZ);

			mesh.meshAABB = { minPoint, maxPoint };
			mesh.aabbVertices.resize(8);

			mesh.aabbVertices[0] = Vertex{ {minPoint.x, maxPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right back
			mesh.aabbVertices[1] = Vertex{ {maxPoint.x, maxPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right back
			mesh.aabbVertices[2] = Vertex{ {maxPoint.x, minPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right front
			mesh.aabbVertices[3] = Vertex{ {minPoint.x, minPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right front

			mesh.aabbVertices[4] = Vertex{ {minPoint.x, maxPoint.y, maxPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top left back
			mesh.aabbVertices[5] = Vertex{ {maxPoint.x, maxPoint.y, maxPoint.z}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // bottom left back
			mesh.aabbVertices[6] = Vertex{ {maxPoint.x, minPoint.y, maxPoint.z}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // top left front 
			mesh.aabbVertices[7] = Vertex{ {minPoint.x, minPoint.y, maxPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom left front
		
		}

		CreateBuffers();
	}

	std::string fixFilePath(std::string str) {
		for (int i = 0; i < str.size(); i++) {
			if (i != str.size() - 1) {
				if (str[i] == '\\') {
					str[i] = '/';
				}
			}
		}
		return str;
	}

	void Model::LoadFBXModel(const std::string& filepath) {
		Assimp::Importer importer;

		const char* pathBegin = filepath.c_str();
		const char* pathEnd = std::strrchr(pathBegin, '/');

		const std::string prefix = pathEnd ? std::string(pathBegin, pathEnd + 1) : "";

		const aiScene* scene = importer.ReadFile(filepath,
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType |
			aiProcess_GenNormals | 
			aiProcess_LimitBoneWeights |
			aiProcess_ImproveCacheLocality | 
			aiProcess_RemoveRedundantMaterials | 
			aiProcess_SplitLargeMeshes | 
			aiProcess_FindInvalidData | 
			aiProcess_OptimizeMeshes | 
			aiProcess_ValidateDataStructure | 
			aiProcess_OptimizeGraph); 

		m_Scene = scene;

		
		for (int i = 0; i < scene->mNumMaterials; i++) {
			Material mi;
			mi.materialName = scene->mMaterials[i]->GetName().C_Str();
			aiColor3D color(0.f, 0.f, 0.f);
			scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			mi.diffuseColour = glm::vec3(color.r, color.g, color.b);

			aiString diffPath;
			scene->mMaterials[i]->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffPath);
			std::string dPath = fixFilePath(diffPath.C_Str());
			if (dPath == "") {
				mi.diffuseTexturePath = "../resources/textures/jpeg/sponza_floor_a_diff.jpg";
			}
			else {
				mi.diffuseTexturePath = prefix + dPath;
			}

			materials.emplace_back(std::move(mi));
		}

		const std::string defaultTexture = "../resources/textures/jpeg/sponza_floor_a_diff.jpg";
		loadedTextures.resize(materials.size());
		for (int i = 0; i < materials.size(); i++)
		{
			if (materials[i].diffuseTexturePath != "")
			{
				Image texture = Enigma::CreateTexture(context, materials[i].diffuseTexturePath);
				loadedTextures[i] = std::move(texture);
			}
			else
			{
				Image defaultTex = Enigma::CreateTexture(context, defaultTexture);
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
			imageInfo.sampler = Enigma::defaultSampler;

			imageinfos.emplace_back(std::move(imageInfo));

		}
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet[0];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(imageinfos.size());
		descriptorWrite.pImageInfo = imageinfos.data();

		vkUpdateDescriptorSets(context.device, 1, &descriptorWrite, 0, nullptr);

		for (auto& mesh : meshes)
		{
			std::vector<Vertex> verts;
			verts.insert(verts.begin(), mesh.vertices.begin(), mesh.vertices.end());

			float minX = std::numeric_limits<float>::max();
			float minY = std::numeric_limits<float>::max();
			float minZ = std::numeric_limits<float>::max();

			float maxX = -std::numeric_limits<float>::max();
			float maxY = -std::numeric_limits<float>::max();
			float maxZ = -std::numeric_limits<float>::max();

			for (unsigned int i = 0; i < verts.size(); i++)
			{
				minX = std::min(minX, verts[i].pos.x);
				minY = std::min(minY, verts[i].pos.y);
				minZ = std::min(minZ, verts[i].pos.z);

				maxX = std::max(maxX, verts[i].pos.x);
				maxY = std::max(maxY, verts[i].pos.y);
				maxZ = std::max(maxZ, verts[i].pos.z);
			}

			glm::vec3 minPoint = glm::vec3(minX, minY, minZ);
			glm::vec3 maxPoint = glm::vec3(maxX, maxY, maxZ);

			mesh.meshAABB = { minPoint, maxPoint };
			mesh.aabbVertices.resize(8);

			mesh.aabbVertices[0] = Vertex{ {minPoint.x, maxPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right back
			mesh.aabbVertices[1] = Vertex{ {maxPoint.x, maxPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right back
			mesh.aabbVertices[2] = Vertex{ {maxPoint.x, minPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right front
			mesh.aabbVertices[3] = Vertex{ {minPoint.x, minPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right front

			mesh.aabbVertices[4] = Vertex{ {minPoint.x, maxPoint.y, maxPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top left back
			mesh.aabbVertices[5] = Vertex{ {maxPoint.x, maxPoint.y, maxPoint.z}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // bottom left back
			mesh.aabbVertices[6] = Vertex{ {maxPoint.x, minPoint.y, maxPoint.z}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // top left front 
			mesh.aabbVertices[7] = Vertex{ {minPoint.x, minPoint.y, maxPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom left front
		}

		CreateBuffers();
	}

	void Model::CreateBuffers()
	{
		for (auto& mesh : meshes)
		{
			VkDeviceSize vertexSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();

			mesh.vertexBuffer = CreateBuffer(context.allocator, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

			// We can't directly add data to GPU memory
			// need to add data to CPU visible memory first and then copy into GPU memory
			Buffer stagingBuffer = CreateBuffer(context.allocator, vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);

			void* data = nullptr;
			ENIGMA_VK_CHECK(vmaMapMemory(context.allocator.allocator, stagingBuffer.allocation, &data), "Failed to map staging buffer memory while loading model.");
			std::memcpy(data, mesh.vertices.data(), vertexSize);
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

			vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &copy);

			VkBufferMemoryBarrier bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.buffer = mesh.vertexBuffer.buffer;
			bufferBarrier.size = VK_WHOLE_SIZE;
			bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

			// End the command buffer and submit it
			Enigma::EndAndSubmitCommandBuffer(context, cmd);

			vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
			vkDestroyCommandPool(context.device, commandPool, nullptr);
		}


		for (auto& mesh : meshes)
		{
			VkDeviceSize indexSize = sizeof(mesh.indices[0]) * mesh.indices.size();
			mesh.indexBuffer = CreateBuffer(context.allocator, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

			// We can't directly add data to GPU memory
			// need to add data to CPU visible memory first and then copy into GPU memory
			Buffer stagingBuffer = CreateBuffer(context.allocator, indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);

			void* data = nullptr;
			ENIGMA_VK_CHECK(vmaMapMemory(context.allocator.allocator, stagingBuffer.allocation, &data), "Failed to map staging buffer memory while loading model.");
			std::memcpy(data, mesh.indices.data(), indexSize);
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

			vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.indexBuffer.buffer, 1, &copy);

			VkBufferMemoryBarrier bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.buffer = mesh.indexBuffer.buffer;
			bufferBarrier.size = VK_WHOLE_SIZE;
			bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

			// End the command buffer and submit it
			Enigma::EndAndSubmitCommandBuffer(context, cmd);

			vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
			vkDestroyCommandPool(context.device, commandPool, nullptr);
		}


		for (auto& mesh : meshes)
		{
			VkDeviceSize vertexSize = sizeof(mesh.aabbVertices[0]) * mesh.aabbVertices.size();
			mesh.AABB_buffer = CreateBuffer(context.allocator, vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

			Buffer stagingBuffer = CreateBuffer(context.allocator, vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);

			void* data = nullptr;
			ENIGMA_VK_CHECK(vmaMapMemory(context.allocator.allocator, stagingBuffer.allocation, &data), "Failed to map staging buffer memory while loading model.");
			std::memcpy(data, mesh.aabbVertices.data(), vertexSize);
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

			vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.AABB_buffer.buffer, 1, &copy);

			VkBufferMemoryBarrier bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.buffer = mesh.AABB_buffer.buffer;
			bufferBarrier.size = VK_WHOLE_SIZE;
			bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

			// End the command buffer and submit it
			Enigma::EndAndSubmitCommandBuffer(context, cmd);

			vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
			vkDestroyCommandPool(context.device, commandPool, nullptr);
		}

		for (auto& mesh : meshes)
		{
			VkDeviceSize indexSize = sizeof(indices[0]) * indices.size();
			mesh.AABB_indexBuffer = CreateBuffer(context.allocator, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

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

			vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.AABB_indexBuffer.buffer, 1, &copy);

			VkBufferMemoryBarrier bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.buffer = mesh.AABB_indexBuffer.buffer;
			bufferBarrier.size = VK_WHOLE_SIZE;
			bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

			// End the command buffer and submit it
			Enigma::EndAndSubmitCommandBuffer(context, cmd);

			vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
			vkDestroyCommandPool(context.device, commandPool, nullptr);
		}
	}

	// Call to draw the model
	void Model::Draw(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline aabPipeline)
	{
		for (auto& mesh : meshes)
		{
			ModelPushConstant push = {};
			push.model = glm::mat4(1.0f);
			if (player) {
				push.model = glm::translate(push.model, this->translation + glm::vec3(0.f, 8.3f, 0.f));
				//push.model = rotMatrix;
				push.model = glm::rotate(push.model, glm::radians(this->rotationY), glm::vec3(0, 1, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationX), glm::vec3(1, 0, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationZ), glm::vec3(0, 0, 1));
			}
			else if (equipment) {
				push.model = glm::translate(push.model, this->translation + offset);
				push.model = rotMatrix;
				push.model = glm::rotate(push.model, glm::radians(this->rotationY), glm::vec3(0, 1, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationX), glm::vec3(1, 0, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationZ), glm::vec3(0, 0, 1));
			}
			else {
				push.model = glm::translate(push.model, this->translation);
				push.model = rotMatrix;
				push.model = glm::rotate(push.model, glm::radians(this->rotationY), glm::vec3(0, 1, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationX), glm::vec3(1, 0, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationZ), glm::vec3(0, 0, 1));
			}
			push.model = glm::scale(push.model, this->scale);
			push.textureIndex = mesh.materialIndex;
			push.isTextured = mesh.textured;

			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPushConstant), &push);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_descriptorSet[0], 0, nullptr);

			VkDeviceSize offset[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, offset);

			vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
		}

		for (auto& mesh : meshes) {

			ModelPushConstant push = {};
			push.model = glm::mat4(1.0f);
			if (player) {
				push.model = glm::translate(push.model, this->translation + glm::vec3(0.f, 8.3f, 0.f));
				//push.model = rotMatrix;
				push.model = glm::rotate(push.model, glm::radians(this->rotationY), glm::vec3(0, 1, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationX), glm::vec3(1, 0, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationZ), glm::vec3(0, 0, 1));
			}
			else if (equipment) {
				push.model = glm::translate(push.model, this->translation + offset);
				push.model = rotMatrix;
				push.model = glm::rotate(push.model, glm::radians(this->rotationY), glm::vec3(0, 1, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationX), glm::vec3(1, 0, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationZ), glm::vec3(0, 0, 1));
			}
			else {
				push.model = glm::translate(push.model, this->translation);
				push.model = rotMatrix;
				push.model = glm::rotate(push.model, glm::radians(this->rotationY), glm::vec3(0, 1, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationX), glm::vec3(1, 0, 0));
				push.model = glm::rotate(push.model, glm::radians(this->rotationZ), glm::vec3(0, 0, 1));
			}
			push.model = glm::scale(push.model, this->scale);
			push.textureIndex = mesh.materialIndex;
			push.isTextured = mesh.textured;

			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPushConstant), &push);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, aabPipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_descriptorSet[0], 0, nullptr);

			VkDeviceSize offset[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.AABB_buffer.buffer, offset);
			//vkCmdDraw(cmd, mesh.aabbVertices.size(), 1, 0, 0);

			vkCmdBindIndexBuffer(cmd, mesh.AABB_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}
	}
    //==========================================================================
    void Model::LoadModelAssimp(const std::string& filepath) {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(
            filepath, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                          aiProcess_SortByPType | aiProcess_GenNormals | aiProcess_LimitBoneWeights |
                          aiProcess_ImproveCacheLocality | aiProcess_RemoveRedundantMaterials |
                          aiProcess_SplitLargeMeshes | aiProcess_FindInvalidData | aiProcess_OptimizeMeshes |
                          aiProcess_ValidateDataStructure | aiProcess_OptimizeGraph);

		if(!scene){
			fprintf(stderr,"failed to load model %s\n",filepath.c_str());
			return;
		}
        m_Scene = scene;

		loadMaterials();
        // load mesh
        //  process each mesh located at the current node
        for (unsigned int i = 0; i < scene->mNumMeshes; i++) processMesh(scene->mMeshes[i]);

        // process node
        int tempIndex = 0;
        processNode(scene->mRootNode, &rootNode, tempIndex);
		loadBones();	
		processAnimation();

        CreateBuffers();

        auto tempM = scene->mRootNode->mTransformation.Inverse().Transpose();
        memcpy(&globalInverseTransform, &tempM, sizeof(glm::mat4));

        createBoneTransformBuffer();
    }
    void Model::updateAnimation(float deltaTime, int index){
        auto&& animation = animations2[index];
		float timeInTicks = deltaTime * animation.ticksPerSecond; // 计算当前时间增量对应的tick数
		float animationTime = fmod(timeInTicks, animation.duration); // 根据动画持续时间循环计算动画当前时间

        for (auto &e : animation.channel) {
            e.node->translation = interpolateTranslation(animationTime,&e);
            e.node->rotation = interpolateRotation(animationTime,&e);
            e.node->scale = interpolateScale(animationTime,&e);
        }
        rootNode.Update();
        updateBoneTransforms();
    }
    void Model::createBoneTransformBuffer() {
        if(boneTransforms.empty())return;
        // VkDeviceSize vertexSize = sizeof(glm::mat4) * boneTransforms.size();
        VkDeviceSize vertexSize = sizeof(glm::mat4) * 300;

        boneTransformBuffer = CreateBuffer(
            context.allocator, vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO);

        void* data = nullptr;
        ENIGMA_VK_CHECK(vmaMapMemory(context.allocator.allocator, boneTransformBuffer.allocation, &data),
                        "Failed to map staging buffer memory while loading model.");
        std::memcpy(data, boneTransforms.data(), vertexSize);
        vmaUnmapMemory(context.allocator.allocator, boneTransformBuffer.allocation);

        //create descriptor setlayout
        AllocateDescriptorSets(context, Enigma::descriptorPool, Enigma::boneTransformDescriptorLayout, 1,
                               boneTransformDescriptorSet);

        VkDescriptorBufferInfo bufferInfo{boneTransformBuffer.buffer, 0, vertexSize};

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = boneTransformDescriptorSet[0];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(context.device, 1, &descriptorWrite, 0, nullptr);
    }
    void Model::processNode(aiNode* node, Node* dstNode, int& index) {
        aiVector3D scaling;
        aiQuaternion rotation;
        aiVector3D position;
        node->mTransformation.Decompose(scaling, rotation, position);
        dstNode->index = index;
        ++index;

        dstNode->translation = { position[0], position[1], position[2] };
        dstNode->scale = { scaling[0], scaling[1], scaling[2] };
        dstNode->rotation = { rotation.w, rotation.x, rotation.y, rotation.z };

        dstNode->meshIndices.insert(dstNode->meshIndices.end(), &node->mMeshes[0], &node->mMeshes[node->mNumMeshes]);
        dstNode->name = node->mName.C_Str();
        tempNodeMap[dstNode->name] = dstNode;

        dstNode->Update(false);
        boneTransforms.emplace_back(dstNode->globalMatrix);

        for (auto e : dstNode->meshIndices) {
            auto& a = meshes[e].meshAABB.min;
            auto& b = meshes[e].meshAABB.max;
            auto p0 = glm::vec3(dstNode->globalMatrix * glm::vec4(a, 1));
            auto p1 = glm::vec3(dstNode->globalMatrix * glm::vec4(b, 1));
            auto p2 = glm::vec3(dstNode->globalMatrix * glm::vec4(b.x, a.y, a.z, 1));
            auto p3 = glm::vec3(dstNode->globalMatrix * glm::vec4(a.x, b.y, a.z, 1));
            auto p4 = glm::vec3(dstNode->globalMatrix * glm::vec4(a.x, a.y, b.z, 1));
            auto p5 = glm::vec3(dstNode->globalMatrix * glm::vec4(b.x, b.y, a.z, 1));
            auto p6 = glm::vec3(dstNode->globalMatrix * glm::vec4(a.x, b.y, b.z, 1));
            auto p7 = glm::vec3(dstNode->globalMatrix * glm::vec4(b.x, a.y, b.z, 1));

            m_AABB.max = glm::max(m_AABB.max, p0);
            m_AABB.max = glm::max(m_AABB.max, p1);
            m_AABB.max = glm::max(m_AABB.max, p2);
            m_AABB.max = glm::max(m_AABB.max, p3);
            m_AABB.max = glm::max(m_AABB.max, p4);
            m_AABB.max = glm::max(m_AABB.max, p5);
            m_AABB.max = glm::max(m_AABB.max, p6);
            m_AABB.max = glm::max(m_AABB.max, p7);

            m_AABB.min = glm::min(m_AABB.min, p0);
            m_AABB.min = glm::min(m_AABB.min, p1);
            m_AABB.min = glm::min(m_AABB.min, p2);
            m_AABB.min = glm::min(m_AABB.min, p3);
            m_AABB.min = glm::min(m_AABB.min, p4);
            m_AABB.min = glm::min(m_AABB.min, p5);
            m_AABB.min = glm::min(m_AABB.min, p6);
            m_AABB.min = glm::min(m_AABB.min, p7);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            dstNode->children.emplace_back(std::make_shared<Node>());
            dstNode->children.back()->parent = dstNode;
            processNode(node->mChildren[i], dstNode->children.back().get(), index);
        }
    }
    void Model::processMesh(aiMesh* mesh) {
		Mesh tempMesh;

		std::vector<Vertex> vertices(mesh->mNumVertices);
		std::vector<unsigned int> indices;

		// Process vertices: positions, normals, and texture coordinates
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex;
			// Processing position
			vertex.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

			// Processing normals
			if (mesh->HasNormals()) {
				vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}

			// Processing texture coordinates
			if (mesh->HasTextureCoords(0)) { // Assumption: only using the first set of UV coordinates
				vertex.tex = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}
			else {
				vertex.tex = glm::vec2(0.0f, 0.0f);
			}

			vertices[i] = vertex;

            tempMesh.meshAABB.max=glm::max(tempMesh.meshAABB.max,vertex.pos);
            tempMesh.meshAABB.min=glm::min(tempMesh.meshAABB.min,vertex.pos);
		}

		tempMesh.aabbVertices.resize(8);

		glm::vec3 minPoint = tempMesh.meshAABB.min;
		glm::vec3 maxPoint = tempMesh.meshAABB.max;

		tempMesh.aabbVertices[0] = Vertex{ {minPoint.x, maxPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right back
		tempMesh.aabbVertices[1] = Vertex{ {maxPoint.x, maxPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right back
		tempMesh.aabbVertices[2] = Vertex{ {maxPoint.x, minPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right front
		tempMesh.aabbVertices[3] = Vertex{ {minPoint.x, minPoint.y, minPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right front

		tempMesh.aabbVertices[4] = Vertex{ {minPoint.x, maxPoint.y, maxPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top left back
		tempMesh.aabbVertices[5] = Vertex{ {maxPoint.x, maxPoint.y, maxPoint.z}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // bottom left back
		tempMesh.aabbVertices[6] = Vertex{ {maxPoint.x, minPoint.y, maxPoint.z}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // top left front 
		tempMesh.aabbVertices[7] = Vertex{ {minPoint.x, minPoint.y, maxPoint.z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom left front

		// Process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) 
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}

		tempMesh.vertices = vertices;
		tempMesh.indices = indices;
		if (mesh->mMaterialIndex >= 0) {
			tempMesh.materialIndex = mesh->mMaterialIndex;
		}
		tempMesh.meshName = mesh->mName.C_Str();
		tempMesh.textured = mesh->HasTextureCoords(0);

		meshes.push_back(std::move(tempMesh));
	}
    void Model::processAnimation() {
		for (unsigned int i = 0; i < m_Scene->mNumAnimations; ++i) {
            animations2.emplace_back();
            animations2.back().ticksPerSecond =
                m_Scene->mAnimations[i]->mTicksPerSecond == 0 ? 30.f : m_Scene->mAnimations[i]->mTicksPerSecond;
            animations2.back().duration = m_Scene->mAnimations[i]->mDuration;

            for (unsigned int j = 0; j < m_Scene->mAnimations[i]->mNumChannels; ++j) {
                auto channel = m_Scene->mAnimations[i]->mChannels[j];
                auto node = tempNodeMap.at(channel->mNodeName.C_Str());
                auto dstChannel = const_cast<NodeAnim*>(findNodeAnim(animations2.back(),node->name));
				if(!dstChannel){
					animations2.back().channel.emplace_back();
					dstChannel = &animations2.back().channel.back();
					dstChannel->nodeName = node->name;
                    dstChannel->node=node;
				}

                for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k) {
                    dstChannel->positions.emplace_back(KeyPosition{
                        float(channel->mPositionKeys[k].mTime),
                        glm::vec3(channel->mPositionKeys[k].mValue[0], channel->mPositionKeys[k].mValue[1],
                                  channel->mPositionKeys[k].mValue[2]) });
                }
                for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k) {
                    dstChannel->rotations.emplace_back(KeyRotation{
                        float(channel->mRotationKeys[k].mTime),
                        glm::quat(channel->mRotationKeys[k].mValue.w, channel->mRotationKeys[k].mValue.x,
                                  channel->mRotationKeys[k].mValue.y, channel->mRotationKeys[k].mValue.z) });
                }
                for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k) {
                    dstChannel->scales.emplace_back(
                        KeyScale{ float(channel->mScalingKeys[k].mTime),
                                      glm::vec3(channel->mScalingKeys[k].mValue[0], channel->mScalingKeys[k].mValue[1],
                                                channel->mScalingKeys[k].mValue[2]) });
                }
            }
        }
	}
    void Model::loadBones() {
		//load mesh bones
        for (unsigned int i = 0; i < m_Scene->mNumMeshes; i++) {
            aiMesh* mesh = m_Scene->mMeshes[i];
            auto& dstMesh = meshes[i];
            for (int j = 0; j < mesh->mNumBones; ++j) {
                auto bone = mesh->mBones[j];
                auto dstNode = tempNodeMap.at(bone->mName.C_Str());
                auto offsetmatrix = bone->mOffsetMatrix.Transpose();
                memcpy(&dstNode->boneOffsetMatrix, &offsetmatrix, sizeof(glm::mat4));

                auto nodeIndex = dstNode->index;
                for (int k = 0; k < bone->mNumWeights; ++k) {
                    for (int m = 0; m < 4; ++m) {
                        auto& weight = dstMesh.vertices[bone->mWeights[k].mVertexId].weights[m];
                        if (weight == 0) {
                            dstMesh.vertices[bone->mWeights[k].mVertexId].boneIDs[m] = nodeIndex;
                            weight = bone->mWeights[k].mWeight;
                            break;
                        }
                    }
                }
            }
            // dstMesh.setupMesh();
        }
	}
    void Model::loadMaterials(){
        const char* pathBegin = m_filePath.c_str();
        const char* pathEnd = std::strrchr(pathBegin, '/');

        const std::string prefix = pathEnd ? std::string(pathBegin, pathEnd + 1) : "";

		for (int i = 0; i < m_Scene->mNumMaterials; i++) {
			Material mi;
			mi.materialName = m_Scene->mMaterials[i]->GetName().C_Str();
			aiColor3D color(0.f, 0.f, 0.f);
			m_Scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			mi.diffuseColour = glm::vec3(color.r, color.g, color.b);

			aiString diffPath;
			m_Scene->mMaterials[i]->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffPath);
			std::string dPath = fixFilePath(diffPath.C_Str());
			if (dPath == "") {
				mi.diffuseTexturePath = "../resources/textures/jpeg/sponza_floor_a_diff.jpg";
			}
			else {
				mi.diffuseTexturePath = prefix + dPath;
			}

			materials.emplace_back(std::move(mi));
		}

		loadedTextures.resize(materials.size());
		for (int i = 0; i < materials.size(); i++)
		{
			Image texture = Enigma::CreateTexture(context, materials[i].diffuseTexturePath);
			loadedTextures[i] = std::move(texture);
		}

		Enigma::AllocateDescriptorSets(context, Enigma::descriptorPool, Enigma::descriptorLayoutModel, 1, m_descriptorSet);

		std::vector<VkDescriptorImageInfo> imageinfos;

		for (size_t i = 0; i < loadedTextures.size(); i++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = loadedTextures[i].imageView;
			imageInfo.sampler = Enigma::defaultSampler;

			imageinfos.emplace_back(std::move(imageInfo));

		}
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
	void Model::Draw2(VkCommandBuffer cmd, VkPipelineLayout layout){
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_descriptorSet[0], 0, nullptr);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, &boneTransformDescriptorSet[0], 0, nullptr);
        drawNode(cmd,layout,&rootNode);
    }
    void Model::drawNode(VkCommandBuffer cmd,VkPipelineLayout layout, Node* node) {
        for (auto e : node->meshIndices) {
            auto&& mesh = meshes[e];
            //draw mesh
			ModelPushConstant push = {};
			push.model = node->globalMatrix;
			push.textureIndex = mesh.materialIndex;
			push.isTextured = mesh.textured;

			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPushConstant), &push);

			VkDeviceSize offset[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, offset);

			vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
        }
        for (auto&& e : node->children) drawNode(cmd, layout, e.get());
    }
	void Model::DrawAABB(VkCommandBuffer cmd, VkPipelineLayout layout){
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_descriptorSet[0], 0, nullptr);
        drawNodeAABB(cmd,layout,&rootNode);
    }
    void Model::drawNodeAABB(VkCommandBuffer cmd,VkPipelineLayout layout,Node* node){
        for (auto e : node->meshIndices) {
            auto&& mesh = meshes[e];
            //draw mesh
			ModelPushConstant push = {};
			push.model = node->globalMatrix;
			push.textureIndex = mesh.materialIndex;
			push.isTextured = mesh.textured;

			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPushConstant), &push);

			VkDeviceSize offset[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.AABB_buffer.buffer, offset);

			vkCmdBindIndexBuffer(cmd, mesh.AABB_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        }
        for (auto&& e : node->children) drawNodeAABB(cmd, layout, e.get());
    }
    void Model::updateBoneTransforms() { 
        updateBoneTransformsHelper(&rootNode); 

        //update buffer
        void* data = nullptr;
        ENIGMA_VK_CHECK(vmaMapMemory(context.allocator.allocator, boneTransformBuffer.allocation, &data),
                        "Failed to map staging buffer memory while loading model.");
        std::memcpy(data, boneTransforms.data(), boneTransforms.size() * sizeof(glm::mat4));
        vmaUnmapMemory(context.allocator.allocator, boneTransformBuffer.allocation);
    }
    void Model::updateBoneTransformsHelper(Node* node) {
        boneTransforms[node->index] = globalInverseTransform * node->globalMatrix * node->boneOffsetMatrix;
        for (auto&& e : node->children) updateBoneTransformsHelper(e.get());
    }
}

