#pragma once

#include <Volk/volk.h>
#include "../Core/Error.h"
#include "../Graphics/VulkanContext.h"
#include "../Graphics/VulkanObjects.h"
#include <GLFW/glfw3.h>
#include "../Graphics/Light.h"
#include <fstream>
#include "../Core/Error.h"
#include "../Core/Engine.h"
#include "VulkanImage.h"

class Model;

namespace Enigma
{
	inline int MAX_FRAMES_IN_FLIGHT = 0;
	inline int currentFrame = 0;

	class Time
	{
		public:
			Time() {
				current = 0.0;
				deltaTime = 0.0;
				lastFrame = 0.0;
			}

			void Update()
			{
				current = glfwGetTime();
				deltaTime = current - lastFrame;
				lastFrame = current;
			}

			double deltaTime;
			double lastFrame;
			double current;
	};


	struct CameraTransform
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;

		float fov = 45.0f;
		float nearPlane = 1.0f;
		float farPlane = 1000.0f;
	};

	struct World
	{
		std::vector<Light> Lights;
		std::vector<Model*> Meshes;
	};

	struct Scene
	{
		
	};

	inline VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	inline VkDescriptorSetLayout sceneDescriptorLayout = VK_NULL_HANDLE;
	inline VkDescriptorSetLayout descriptorLayoutModel = VK_NULL_HANDLE;

	inline Sampler sampler;
	inline Image depth;

	inline VkImage depthimg;
	inline VkImageView depthimageview;
	inline VmaAllocation depthimageallocation;

	inline VkSampler defaultSampler;

	inline VkPipeline draw_line_list;
}

namespace Enigma
{

	inline VkDescriptorSetLayout CreateDescriptorSetLayout(const VulkanContext& context, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
	{
		VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		info.bindingCount = static_cast<uint32_t>(bindings.size());
		info.pBindings = bindings.data();

		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		
		ENIGMA_VK_CHECK(vkCreateDescriptorSetLayout(context.device, &info, nullptr, &layout), "Failed to crate descriptor set layout");

		return layout;
	}

	inline void AllocateDescriptorSets(const VulkanContext& context, VkDescriptorPool descriptorPool, const VkDescriptorSetLayout descriptorLayout, uint32_t setCount, std::vector<VkDescriptorSet>& descriptorSet)
	{
		std::vector<VkDescriptorSetLayout> setLayout(Enigma::MAX_FRAMES_IN_FLIGHT, descriptorLayout);
		
		VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = setCount;
		allocInfo.pSetLayouts = setLayout.data();

		descriptorSet.resize(Enigma::MAX_FRAMES_IN_FLIGHT);

		ENIGMA_VK_CHECK(vkAllocateDescriptorSets(context.device, &allocInfo, descriptorSet.data()), "Failed to allocate descriptor sets");
	}

	// Define a descriptor set layout binding 
	// binding = which index slot to bind to e.g. 0,1,2..
	// type = what is the layout binding type e.g. uniform or sampler
	// shadeStage = which shader will this be accessed and used in? e.g. vertex, fragment or both
	inline VkDescriptorSetLayoutBinding CreateDescriptorBinding(uint32_t binding, uint32_t count, VkDescriptorType type, VkShaderStageFlags shaderStage)
	{
		VkDescriptorSetLayoutBinding bindingLayout = {};
		bindingLayout.binding = binding;
		bindingLayout.descriptorType = type;
		bindingLayout.descriptorCount = count;
		bindingLayout.stageFlags = shaderStage;

		return bindingLayout;
	}

	// Update buffer descriptor
	inline void UpdateDescriptorSet(const VulkanContext& context, uint32_t binding, VkDescriptorBufferInfo bufferInfo, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType)
	{
		VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorType;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(context.device, 1, &descriptorWrite, 0, nullptr);
	}

	// Update image descriptor
	inline void UpdateDescriptorSet(const VulkanContext& context, uint32_t binding, VkDescriptorImageInfo imageInfo, VkDescriptorSet descriptorSet, VkDescriptorType descriptorType)
	{
		VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorType;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(context.device, 1, &descriptorWrite, 0, nullptr);
	}

	inline VkCommandBuffer AllocateCommandBuffer(const VulkanContext& context, VkCommandPool cmdPool)
	{
		VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandBufferCount = 1;
		allocInfo.commandPool = cmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		
		VkCommandBuffer cmd = VK_NULL_HANDLE;
		ENIGMA_VK_CHECK(vkAllocateCommandBuffers(context.device, &allocInfo, &cmd), "Failed to allocate command buffer");

		return cmd;
	}

	inline void BeginCommandBuffer(VkCommandBuffer cmd)
	{
		VkCommandBufferBeginInfo begin{};
		begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		ENIGMA_VK_CHECK(vkBeginCommandBuffer(cmd, &begin), "Failed to begin recording command buffer");
	}

	inline Fence CreateFence(VkDevice device)
	{
		VkFenceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkFence fence = VK_NULL_HANDLE;
		ENIGMA_VK_CHECK(vkCreateFence(device, &info, nullptr, &fence), "Failed to create fence");

		return Fence(device, fence);
	}

	inline void EndAndSubmitCommandBuffer(const VulkanContext& context, VkCommandBuffer cmd)
	{
		vkEndCommandBuffer(cmd);

		Fence complete = CreateFence(context.device);

		vkResetFences(context.device, 1, &complete.handle);

		VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd;

		ENIGMA_VK_CHECK(vkQueueSubmit(context.graphicsQueue, 1, &submit, complete.handle), "Failed to submit command buffer");
	
		vkWaitForFences(context.device, 1, &complete.handle, VK_TRUE, UINT64_MAX);
	}

	inline Semaphore CreateSemaphore(VkDevice device)
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkSemaphore semaphore = VK_NULL_HANDLE;
		ENIGMA_VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore), "Failed to create Semaphore");

		return Semaphore{ device, semaphore };
	}

	inline CommandPool CreateCommandPool(VkDevice device, VkCommandPoolCreateFlagBits flags, uint32_t queueFamilyIndex)
	{
		VkCommandPoolCreateInfo cmdPool{};
		cmdPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPool.flags = flags;
		cmdPool.queueFamilyIndex = queueFamilyIndex;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		ENIGMA_VK_CHECK(vkCreateCommandPool(device, &cmdPool, nullptr, &commandPool), "Failed to create command pool");

		return CommandPool{ device, commandPool };
	}

	inline ShaderModule CreateShaderModule(const std::string& filename, VkDevice device)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open shader file.");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		VkShaderModuleCreateInfo shadermoduleInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		shadermoduleInfo.codeSize = buffer.size();
		shadermoduleInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		if (vkCreateShaderModule(device, &shadermoduleInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module");
		}

		return Enigma::ShaderModule(device, shaderModule);
	}

	inline VkSampler CreateSampler(const VulkanContext& context)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.mipLodBias = 0.f;
		samplerInfo.maxAnisotropy = 16.0f;
		samplerInfo.anisotropyEnable = VK_TRUE;
		
		VkSampler sampler = VK_NULL_HANDLE;
		ENIGMA_VK_CHECK(vkCreateSampler(context.device, &samplerInfo, nullptr, &sampler), "Failed to create sampler");
		
		return sampler;
	}
}
