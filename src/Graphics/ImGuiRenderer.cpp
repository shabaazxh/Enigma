
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "ImGuiRenderer.h"

namespace Enigma
{
	void ImGuiRenderer::Initialize(const VulkanContext& context, VulkanWindow& window)
	{
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = (uint32_t)ImGuiPoolSizes.size();
		pool_info.pPoolSizes = ImGuiPoolSizes.data();

		ENIGMA_VK_CHECK(vkCreateDescriptorPool(context.device, &pool_info, nullptr, &ImGuiRenderer::imGuiDescriptorPool), "Failed to create ImGui descriptor pool.");

		RenderPass(context);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO io = ImGui::GetIO(); (void)io;

		ImGui_ImplGlfw_InitForVulkan(window.window, true);

		ImGui_ImplVulkan_InitInfo info = {};
		info.Instance = context.instance;
		info.PhysicalDevice = context.physicalDevice;
		info.Device = context.device;
		info.Queue = context.graphicsQueue;
		info.QueueFamily = context.graphicsFamilyIndex;
		info.DescriptorPool = ImGuiRenderer::imGuiDescriptorPool;
		info.MinImageCount = 3;
		info.ImageCount = 3;
		info.Subpass = 0;
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		info.RenderPass = imGuiRenderPass;
		
		ImGui_ImplVulkan_Init(&info);

		io.Fonts->AddFontDefault();

		VkCommandPoolCreateInfo cmdPool{};
		cmdPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPool.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		cmdPool.queueFamilyIndex = context.graphicsFamilyIndex;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		ENIGMA_VK_CHECK(vkCreateCommandPool(context.device, &cmdPool, nullptr, &commandPool), "Failed to create command pool for imgui");

		VkCommandBufferAllocateInfo cmdAlloc{};
		cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAlloc.commandPool = commandPool;
		cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdAlloc.commandBufferCount = 1;

		VkCommandBuffer cmd = VK_NULL_HANDLE;
		ENIGMA_VK_CHECK(vkAllocateCommandBuffers(context.device, &cmdAlloc, &cmd), "Failed to allocate command buffers while loading model.");

		Enigma::BeginCommandBuffer(cmd);

		// End the command buffer and submit it
		Enigma::EndAndSubmitCommandBuffer(context, cmd);

		vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
		vkDestroyCommandPool(context.device, commandPool, nullptr);
	}
	void ImGuiRenderer::RenderPass(const VulkanContext& context)
	{
		VkAttachmentDescription attachment{};
		attachment.format = VK_FORMAT_B8G8R8A8_UNORM;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // load the contents of the previous
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store the result
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // not using stencil, use don't care
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // not using stencil, use don't care
		attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout of the resource as it enters render pass
		attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // layout of resource at the end of the render pass

		VkAttachmentDescription attachments[1] = { attachment };

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;

		VkSubpassDependency dependency[1]{};
		// Wait for EXTERNAL render pass to finish outputting
		dependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency[0].srcAccessMask = 0;
		dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		// Destination needs to wait for EXTERNAL to finishing outputting before writing 
		dependency[0].dstSubpass = 0;
		dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkRenderPassCreateInfo rp_info{};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.attachmentCount = 1;
		rp_info.pAttachments = attachments;
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;
		rp_info.dependencyCount = 1;
		rp_info.pDependencies = dependency;

		ENIGMA_VK_CHECK(vkCreateRenderPass(context.device, &rp_info, nullptr, &ImGuiRenderer::imGuiRenderPass), "Failed to create imgui Render Pass.");

	}

	void ImGuiRenderer::Shutdown(const VulkanContext& context)
	{
		// destroy vulkan resources for imgui

		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(context.device, imGuiDescriptorPool, nullptr);
		vkDestroyRenderPass(context.device, imGuiRenderPass, nullptr);
		
		ImGui_ImplGlfw_Shutdown();
	}

	void ImGuiRenderer::Render(VkCommandBuffer cmd, VulkanWindow& window, uint32_t index)
	{
		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();

		VkRenderPassBeginInfo rp_info = {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_info.framebuffer = window.swapchainFramebuffers[index];
		rp_info.renderPass = imGuiRenderPass;
		rp_info.renderArea.extent = window.swapchainExtent;
		rp_info.renderArea.offset = { 0,0 };

		VkClearValue clearValues[1];
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

		rp_info.clearValueCount = 1;
		rp_info.pClearValues = clearValues;

		vkCmdBeginRenderPass(cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
		ImGui_ImplVulkan_RenderDrawData(main_draw_data, cmd);
		vkCmdEndRenderPass(cmd);
	}
}

