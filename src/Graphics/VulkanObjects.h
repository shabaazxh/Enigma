#pragma once

#include <Volk/volk.h>
#include <utility>
#include <cassert>

namespace Enigma
{
	template<typename tParent, typename tHandle>
	using DestroyFn = void(*)(tParent, tHandle, VkAllocationCallbacks const*);

	template<typename tHandle, typename tParent, DestroyFn<tParent, tHandle>& tDestroyFn>
	class VulkanObjectHandle final
	{
	public:
		VulkanObjectHandle() noexcept = default;
		explicit VulkanObjectHandle(tParent, tHandle = VK_NULL_HANDLE) noexcept;

		~VulkanObjectHandle();

		VulkanObjectHandle(const VulkanObjectHandle&) = delete;
		VulkanObjectHandle& operator = (const VulkanObjectHandle&) = delete;

		VulkanObjectHandle(VulkanObjectHandle&&) noexcept;
		VulkanObjectHandle& operator=(VulkanObjectHandle&&) noexcept;
	public:
		tHandle handle = VK_NULL_HANDLE;

	private:
		tParent m_Parent = VK_NULL_HANDLE;
	};

	using CommandPool = VulkanObjectHandle<VkCommandPool, VkDevice, vkDestroyCommandPool>;
	using Fence = VulkanObjectHandle<VkFence, VkDevice, vkDestroyFence>;
	using Semaphore = VulkanObjectHandle<VkSemaphore, VkDevice, vkDestroySemaphore>;

	using Pipeline = VulkanObjectHandle<VkPipeline, VkDevice, vkDestroyPipeline>;
	using PipelineLayout = VulkanObjectHandle<VkPipelineLayout, VkDevice, vkDestroyPipelineLayout>;
	using ShaderModule = VulkanObjectHandle<VkShaderModule, VkDevice, vkDestroyShaderModule>;
	using Sampler = VulkanObjectHandle<VkSampler, VkDevice, vkDestroySampler>;
}

namespace Enigma
{
	template<typename tHandle, typename tParent, DestroyFn<tParent, tHandle>& tDestroyFn>
	inline VulkanObjectHandle<tHandle, tParent, tDestroyFn>::VulkanObjectHandle(tParent parent, tHandle ahandle) noexcept :
		handle{ ahandle }, m_Parent{ parent } {}

	template<typename tHandle, typename tParent, DestroyFn<tParent, tHandle>& tDestroyFn>
	inline VulkanObjectHandle<tHandle, tParent, tDestroyFn>::~VulkanObjectHandle()
	{
		if (handle != VK_NULL_HANDLE)
		{
			assert(m_Parent != VK_NULL_HANDLE);
			tDestroyFn(m_Parent, handle, nullptr);
		}
	}

	template<typename tHandle, typename tParent, DestroyFn<tParent, tHandle>& tDestroyFn>
	inline VulkanObjectHandle<tHandle, tParent, tDestroyFn>::VulkanObjectHandle(VulkanObjectHandle&& other) noexcept
		: handle(std::exchange(other.handle, VK_NULL_HANDLE)),
		m_Parent(std::exchange(other.m_Parent, VK_NULL_HANDLE)) {}


	template<typename tHandle, typename tParent, DestroyFn<tParent, tHandle>& tDestroyFn>
	inline 
	VulkanObjectHandle<tHandle, tParent, tDestroyFn>& VulkanObjectHandle<tHandle, tParent, tDestroyFn>::operator=(VulkanObjectHandle&& other) noexcept
	{
		std::swap(handle, other.handle);
		std::swap(m_Parent, other.m_Parent);
		return *this;
	}

}
