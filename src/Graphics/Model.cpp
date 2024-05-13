#include "Model.h"
#include "VulkanObjects.h"
#include <rapidobj.hpp>
#include <unordered_set>
#include "../Graphics/Common.h"
#include "../Core/Engine.h"

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

	Model::Model(const std::string& filepath, const VulkanContext& context, int filetype, const std::string& name) : 
		m_filePath { filepath }, context{ context }, modelName{name}
	{
		if (filetype == ENIGMA_LOAD_OBJ_FILE)
			LoadOBJModel(filepath);
		else if (filetype == ENIGMA_LOAD_FBX_FILE)
			LoadFBXModel(filepath);
	}
	Model::Model(const std::string& filepath, const VulkanContext& context, int filetype) : m_filePath{filepath}, context{context}
	{
		if (filetype == ENIGMA_LOAD_OBJ_FILE)
			LoadOBJModel(filepath);
		else if (filetype == ENIGMA_LOAD_FBX_FILE)
			LoadFBXModel(filepath);
	}

	void Model::LoadOBJModel(const std::string& filepath)
	{
		// Load the obj file
		rapidobj::Result result = rapidobj::ParseFile(filepath.c_str());
		if (result.error) {
			std::cout << "RapidObj: " << result.error.code.message() << std::endl;
			ENIGMA_ERROR("Failed to load model.");
			throw std::runtime_error("Failed to load model");
		}

		// obj can have non triangle faces. Triangulate will triangulate
		// non triangle faces
		for (const auto& shape : result.shapes) {
			if (shape.name == "Navmesh") {
				std::vector<int> vertices;
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
		rapidobj::Triangulate(result);

		// store the prefix to the obj file
		const char* pathBegin = filepath.c_str();
		const char* pathEnd = std::strrchr(pathBegin, '/');

		const std::string prefix = pathEnd ? std::string(pathBegin, pathEnd + 1) : "";

		m_filePath = filepath;
		modelName = std::string(pathEnd);

		// Find the materials for this model (searching for diffuse only at the moement)
		for (const auto& mat : result.materials)
		{
			Material mi;
			mi.materialName = mat.name;
			mi.diffuseColour = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);

			if (!mat.diffuse_texname.empty()) 
			{
				mi.diffuseTexturePath = prefix + mat.diffuse_texname;
			}
			
			if (!mat.metallic_texname.empty())
			{
				mi.metallicTexturePath = prefix + mat.metallic_texname;
			}

			if (!mat.roughness_texname.empty())
			{
				mi.roughnessTexturePath = prefix + mat.roughness_texname;
			}

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
			
			// process vertices for materials which are active
			for (const auto matID : activeMaterials)
			{
				Vertex vertex{};
				const bool diffuse = !materials[matID].diffuseTexturePath.empty();
				const bool metallic = !materials[matID].metallicTexturePath.empty();
				const bool roughness = !materials[matID].roughnessTexturePath.empty();

				if (!diffuse)
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

						result.attributes.normals[idx.normal_index * 3 + 0],
						result.attributes.normals[idx.normal_index * 3 + 1],
						result.attributes.normals[idx.normal_index * 3 + 2]
						});

					if (diffuse)
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
				mesh.textured = diffuse;
				mesh.hasMetallic = metallic;
				mesh.hadRoughness = roughness;
				meshes.emplace_back(std::move(mesh));
			}

		}

		
		// need to store it at mesh index not material index when pushing into loaded exxtures
		// if a diffuse texture cannot be found, a pink texure is used as a placeholder
		// pink is used since it's easily visible in the scene to indicate there is a mesh issue
		const std::string defaultTexture = "../resources/default_texture.jpg";
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

		MetallicTextures.resize(materials.size());
		for (int i = 0; i < materials.size(); i++)
		{
			if (materials[i].roughnessTexturePath != "")
			{
				Image texture = Enigma::CreateTexture(context, materials[i].metallicTexturePath);
				MetallicTextures[i] = std::move(texture);
			}
			else
			{
				Image defaultTex = Enigma::CreateTexture(context, defaultTexture);
				MetallicTextures[i] = std::move(defaultTex);
			}
		}

		Enigma::AllocateDescriptorSets(context, Enigma::descriptorPool, Enigma::descriptorLayoutModel, 1, m_descriptorSet);

		std::vector<VkDescriptorImageInfo> imageinfos;

		for (size_t i = 0; i < loadedTextures.size(); i++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = loadedTextures[i].imageView;
			imageInfo.sampler = Enigma::repeatSampler;

			imageinfos.emplace_back(std::move(imageInfo));

		}

		std::vector<VkDescriptorImageInfo> metallic_image_infos;
		for (size_t i = 0; i < MetallicTextures.size(); i++)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = MetallicTextures[i].imageView;
			imageInfo.sampler = Enigma::defaultSampler;

			metallic_image_infos.emplace_back(std::move(imageInfo));
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

		VkWriteDescriptorSet metallic_descriptorWrite{};
		metallic_descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		metallic_descriptorWrite.dstSet = m_descriptorSet[0];
		metallic_descriptorWrite.dstBinding = 1;
		metallic_descriptorWrite.dstArrayElement = 0;
		metallic_descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		metallic_descriptorWrite.descriptorCount = static_cast<uint32_t>(metallic_image_infos.size());
		metallic_descriptorWrite.pImageInfo = metallic_image_infos.data();

		std::vector<VkWriteDescriptorSet> sets = { descriptorWrite, metallic_descriptorWrite };

		vkUpdateDescriptorSets(context.device, (uint32_t)sets.size(), sets.data(), 0, nullptr);
	
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

			mesh.aabbVertices[0] = Vertex{ {minPoint.x, maxPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right back
			mesh.aabbVertices[1] = Vertex{ {maxPoint.x, maxPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right back
			mesh.aabbVertices[2] = Vertex{ {maxPoint.x, minPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right front
			mesh.aabbVertices[3] = Vertex{ {minPoint.x, minPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right front

			mesh.aabbVertices[4] = Vertex{ {minPoint.x, maxPoint.y, maxPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top left back
			mesh.aabbVertices[5] = Vertex{ {maxPoint.x, maxPoint.y, maxPoint.z}, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // bottom left back
			mesh.aabbVertices[6] = Vertex{ {maxPoint.x, minPoint.y, maxPoint.z}, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // top left front 
			mesh.aabbVertices[7] = Vertex{ {minPoint.x, minPoint.y, maxPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom left front
		
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

		const aiScene* scene = importer.ReadFile(filepath, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
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

		for (int i = 0; i < scene->mNumMeshes; i++) {
			Mesh mesh;
			std::unordered_map<Vertex, uint32_t> uniqueVertices;
			for (int j = 0; j < scene->mMeshes[i]->mNumVertices; j++) {
				Vertex vert;
				aiVector3D assimpVert;
				assimpVert = scene->mMeshes[i]->mVertices[j];
				vert.pos = glm::vec3(assimpVert.x, assimpVert.y, assimpVert.z);

				aiVector3D assimpTexVert;
				assimpTexVert = scene->mMeshes[i]->mTextureCoords[0][j];
				vert.tex = glm::vec2(assimpTexVert.x, assimpTexVert.y);

				if (scene->mMeshes[i]->HasVertexColors(i)) {
					aiColor4D assimpColVert;
					assimpColVert = scene->mMeshes[i]->mColors[0][j];
					vert.color = glm::vec3(assimpColVert.r, assimpColVert.g, assimpColVert.b);
				}
				
				mesh.vertices.push_back(vert);
			}
			if (scene->HasAnimations()) {
				loadBones(scene->mMeshes[i], mesh.vertices);
			}
			for (int j = 0; j < scene->mMeshes[i]->mNumFaces; j++) {
				for (int k = 0; k < scene->mMeshes[i]->mFaces->mNumIndices; k++) {
					mesh.indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[k]);
				}
			}

			mesh.materialIndex = scene->mMeshes[i]->mMaterialIndex;
			mesh.meshName = scene->mMeshes[i]->mName.C_Str();
			mesh.textured = true;
			meshes.emplace_back(std::move(mesh));
		}

		if (scene->HasAnimations()) {
			hasAnimations = true;
			animations = scene->mAnimations;
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

			mesh.aabbVertices[0] = Vertex{ {minPoint.x, maxPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right back
			mesh.aabbVertices[1] = Vertex{ {maxPoint.x, maxPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right back
			mesh.aabbVertices[2] = Vertex{ {maxPoint.x, minPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top right front
			mesh.aabbVertices[3] = Vertex{ {minPoint.x, minPoint.y, minPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom right front

			mesh.aabbVertices[4] = Vertex{ {minPoint.x, maxPoint.y, maxPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // top left back
			mesh.aabbVertices[5] = Vertex{ {maxPoint.x, maxPoint.y, maxPoint.z}, {0.0f, 0.f, 0.0f},  { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // bottom left back
			mesh.aabbVertices[6] = Vertex{ {maxPoint.x, minPoint.y, maxPoint.z}, {0.0f, 0.f, 0.0f},  { 0.0f, 0.0f },  { 0.0f, 0.0f, 0.0f } }; // top left front 
			mesh.aabbVertices[7] = Vertex{ {minPoint.x, minPoint.y, maxPoint.z }, {0.0f, 0.f, 0.0f}, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }; // bottom left front
		}

		CreateBuffers();
	}

	void Model::loadBones(aiMesh* mesh, std::vector<Vertex>& boneData) {
		for (uint32_t i = 0; i < mesh->mNumBones; i++) {
			uint32_t boneIndex = 0;
			std::string boneName(mesh->mBones[i]->mName.data);

			if (boneMapping.find(boneName) == boneMapping.end()) {
				boneIndex = numBones++;
				//BoneInfo bi;
				//boneInfo.push_back(bi);
			}
			else {
				boneIndex = boneMapping[boneName];
			}

			boneMapping[boneName] = boneIndex;
			//boneInfo[boneIndex].offsetMatrix = aiMat4x4toMat4(mesh->mBones[i]->mOffsetMatrix);

			for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; j++) {
				uint32_t vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
				float weight = mesh->mBones[i]->mWeights[j].mWeight;
				boneData[vertexID].add(boneIndex, weight);
			}
		}
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

		CreateAABBBuffers();
	}

	void Model::CreateAABBBuffers()
	{
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
	void Model::Draw(VkCommandBuffer cmd, VkPipelineLayout layout)
	{
		// Update the AABB

		for (auto& mesh : meshes)
		{
			// scale, rotate, translate -> T * R * S
			ModelPushConstant push = {};
			push.model = glm::mat4(1.0f);
			push.model = glm::translate(push.model, translation);
			push.model = push.model * rotMatrix;
			push.model = glm::rotate(push.model, (float)((this->getXRotation() * 3.141) / 180), glm::vec3(1.f, 0.f, 0.f));
			push.model = glm::rotate(push.model, (float)((this->getYRotation() * 3.141) / 180), glm::vec3(0.f, 1.f, 0.f));
			push.model = glm::rotate(push.model, (float)((this->getZRotation() * 3.141) / 180), glm::vec3(0.f, 0.f, 1.f));
			push.model = glm::scale(push.model, this->scale);
			push.textureIndex = mesh.materialIndex;
			push.isTextured = mesh.textured;

			//UpdateAABB(mesh, push.model);
			//mesh.position = translation;
			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPushConstant), &push);

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_descriptorSet[0], 0, nullptr);

			VkDeviceSize offset[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, offset);

			vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
		}

	}


	void Model::DrawDebug(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline AABBPipeline)
	{
		for (auto& mesh : meshes) {

			ModelPushConstant push = {};
			push.model = glm::mat4(1.0f);
			push.model = glm::translate(push.model, translation);
			push.model = push.model * rotMatrix;
			push.model = glm::rotate(push.model, (float)((this->getXRotation() * 3.141) / 180), glm::vec3(1.f, 0.f, 0.f));
			push.model = glm::rotate(push.model, (float)((this->getYRotation() * 3.141) / 180), glm::vec3(0.f, 1.f, 0.f));
			push.model = glm::rotate(push.model, (float)((this->getZRotation() * 3.141) / 180), glm::vec3(0.f, 0.f, 1.f));
			push.model = glm::scale(push.model, this->scale);
			push.textureIndex = mesh.materialIndex;
			push.isTextured = mesh.textured;

			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPushConstant), &push);

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, AABBPipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_descriptorSet[0], 0, nullptr);

			VkDeviceSize offset[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.AABB_buffer.buffer, offset);

			vkCmdBindIndexBuffer(cmd, mesh.AABB_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}
	}

	void Model::DrawDebug(VkCommandBuffer cmd, VkPipelineLayout layout, VkPipeline AABBPipeline, int index)
	{
		auto &mesh = meshes[index];
		ModelPushConstant push = {};
		push.model = glm::mat4(1.0f);
		push.model = glm::translate(push.model, translation);
		push.model = push.model * rotMatrix;
		push.model = glm::rotate(push.model, (float)((this->getXRotation() * 3.141) / 180), glm::vec3(1.f, 0.f, 0.f));
		push.model = glm::rotate(push.model, (float)((this->getYRotation() * 3.141) / 180), glm::vec3(0.f, 1.f, 0.f));
		push.model = glm::rotate(push.model, (float)((this->getZRotation() * 3.141) / 180), glm::vec3(0.f, 0.f, 1.f));
		push.model = glm::scale(push.model, this->scale);
		push.textureIndex = mesh.materialIndex;
		push.isTextured = mesh.textured;

		vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelPushConstant), &push);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, AABBPipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_descriptorSet[0], 0, nullptr);

		VkDeviceSize offset[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.AABB_buffer.buffer, offset);

		vkCmdBindIndexBuffer(cmd, mesh.AABB_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	}
}

