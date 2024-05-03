#include "Model.h"
#include "VulkanObjects.h"
#include <rapidobj.hpp>
#include <unordered_set>
#include "../Graphics/Common.h"
#include "../Core/Engine.h"
#include <unordered_map>
#include <string>

glm::mat4 aiMat4x4toMat4(aiMatrix4x4 m);

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

		m_Scene = importer.ReadFile(filepath,
			aiProcess_Triangulate |
			aiProcess_FlipUVs |
			aiProcess_CalcTangentSpace);

		if (!m_Scene || !m_Scene->HasMeshes()) {
			std::cerr << "Error loading model: " << filepath << std::endl;
			return;
		}

		m_filePath = filepath.substr(0, filepath.find_last_of('/'));

		processNode(m_Scene->mRootNode, m_Scene);
		
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

	void Model::loadBones(aiMesh* mesh, std::vector<Vertex>& boneData) {
		for (uint32_t i = 0; i < mesh->mNumBones; i++) {
			uint32_t boneIndex = 0;
			std::string boneName(mesh->mBones[i]->mName.data);

			if (boneMapping.find(boneName) == boneMapping.end()) {
				boneIndex = numBones++;
				BoneInfo bi;
				boneInfo.push_back(bi);
			}
			else {
				boneIndex = boneMapping[boneName];
			}

			boneMapping[boneName] = boneIndex;
			boneInfo[boneIndex].offsetMatrix = aiMat4x4toMat4(mesh->mBones[i]->mOffsetMatrix);

			for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; j++) {
				uint32_t vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
				float weight = mesh->mBones[i]->mWeights[j].mWeight;
				boneData[vertexID].add(boneIndex, weight);
			}
		}
	}

	Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
		std::vector<Vertex> vertices(mesh->mNumVertices);
		std::vector<unsigned int> indices;
		Material material;

		// Process vertex positions, normals, and texture coordinates
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex;
			glm::vec3 vector;  // Convert Assimp data types to glm for processing positions, normals, and texture coordinates

			// Process vertex positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.pos = vector;

			// Process vertex normals
			if (mesh->HasNormals()) {
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.normal = vector;
			}

			// Process texture coordinates
			if (mesh->HasTextureCoords(0)) { // Mesh has texture coordinates
				glm::vec2 tex;
				tex.x = mesh->mTextureCoords[0][i].x;
				tex.y = mesh->mTextureCoords[0][i].y;
				vertex.tex = tex;
			}
			else {
				vertex.tex = glm::vec2(0.0f, 0.0f);
			}

			vertices[i] = vertex;
		}

		// Process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}

		// Process material information if available
		if (mesh->mMaterialIndex >= 0) {
			aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
			aiColor3D color(0.f, 0.f, 0.f);

			/*
			* light
			// Read the diffuse color
			if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
				material.diffuse = glm::vec3(color.r, color.g, color.b);
			}
			// Process more material properties like shininess, opacity, etc.
			// For example: AI_MATKEY_SHININESS, AI_MATKEY_OPACITY
			*/
		}

		if (mesh->HasBones()) {
			loadBones(mesh, vertices);
		}

		Mesh temp;
		temp.vertices = vertices;
		temp.indices = indices;
		temp.meshName = mesh->mName.C_Str();
		return temp;
	}

	void Model::processNode(aiNode* node, const aiScene* scene) {
		// Process all meshes in the node
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back( processMesh(mesh, scene));
		}
		// Process all child nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene);
		}
	}

	/*
	void Model::LoadAnimations(const aiScene* scene) {
		for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
			aiAnimation* anim = scene->mAnimations[i];
		}
	}
	*/

	void Model::updateBoneTransforms() {
		boneTransforms.resize(numBones);
		for (size_t i = 0; i < boneInfo.size(); i++) {
			boneTransforms[i] = boneInfo[i].finalTransformation;
		}
	}

	void Model::updateAnimation(float deltaTime) {
		if (animations.empty())
			return;

		for (auto& anim : animations) {
			float ticksPerSecond = (anim.ticksPerSecond != 0 ? anim.ticksPerSecond : 25.0f); // Get the number of ticks per second, defaulting to 25 if not specified
			float timeInTicks = deltaTime * ticksPerSecond; // Calculate the number of ticks corresponding to the current time increment
			float animationTime = fmod(timeInTicks, anim.duration); // Calculate the current animation time by looping based on the animation duration

			readNodeHierarchy(animationTime, rootNode, glm::mat4(1.0f), anim); // Call readNodeHierarchy with the current animation time, root node, initial transformation matrix, and current animation
		}

		updateBoneTransforms(); // Update bone transformation matrices
	}

	void Model::readNodeHierarchy(float animationTime, Node* node, const glm::mat4& parentTransform, const Animation& anim) {
		glm::mat4 nodeTransform = node->transformation;

		const NodeAnim* nodeAnim = findNodeAnim(anim, node->name); 
		if (nodeAnim) {
			glm::mat4 scalingMat = interpolateScale(animationTime, nodeAnim);
			glm::mat4 rotationMat = interpolateRotation(animationTime, nodeAnim);
			glm::mat4 translationMat = interpolateTranslation(animationTime, nodeAnim);

			nodeTransform = translationMat * rotationMat * scalingMat;
		}

		glm::mat4 globalTransform = parentTransform * nodeTransform;

		if (boneMapping.find(node->name) != boneMapping.end()) {
			unsigned int boneIndex = boneMapping[node->name];
			boneInfo[boneIndex].finalTransformation = globalInverseTransform * globalTransform * boneInfo[boneIndex].offsetMatrix;
		}

		for (auto child : node->children) {
			readNodeHierarchy(animationTime, child, globalTransform, anim);
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
}

glm::mat4 aiMat4x4toMat4(aiMatrix4x4 m) {
	glm::mat4 newM;

	newM[0][0] = m[0][0];
	newM[1][0] = m[1][0];
	newM[2][0] = m[2][0];
	newM[3][0] = m[3][0];
	newM[0][1] = m[0][1];
	newM[1][1] = m[1][1];
	newM[2][1] = m[2][1];
	newM[3][1] = m[3][1];
	newM[0][2] = m[0][2];
	newM[1][2] = m[1][2];
	newM[2][2] = m[2][2];
	newM[3][2] = m[3][2];
	newM[0][3] = m[0][3];
	newM[1][3] = m[1][3];
	newM[2][3] = m[2][3];
	newM[3][3] = m[3][3];

	return newM;
}

