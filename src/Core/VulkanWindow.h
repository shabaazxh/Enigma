#pragma once

#include "../Graphics/VulkanContext.h"
#include "../Graphics/VulkanImage.h"
#include "Camera.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../Core/Engine.h"
#include "../Core/Error.h"
#include "../Core/Settings.h"

namespace Enigma
{
	class VulkanWindow final
	{
		public:
			VulkanWindow(const VulkanContext& vulkanContext);
			~VulkanWindow();

			bool isSwapchainOutdated(VkResult result);

			static void glfw_callback_key_press(GLFWwindow* window, int key, int, int action, int)
			{
				VulkanWindow* windowClass = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));

				if (GLFW_KEY_ESCAPE == key && action == GLFW_PRESS)
				{
					glfwSetWindowShouldClose(window, GLFW_TRUE);
				}

				if (GLFW_KEY_M == key && action == GLFW_PRESS)
				{
					Enigma::isDebug = Enigma::isDebug ? false : true;
				}

				if (glfwGetKey(windowClass->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
				{
					windowClass->camera->SetSpeed(100.0f);
				}
				else {
					windowClass->camera->SetSpeed(50.0f);
				}

				if (glfwGetKey(windowClass->window, GLFW_KEY_P) == GLFW_PRESS)
				{
					isPlayer = !isPlayer;
				}				
				
			}

			static void glfw_callback_mouse(GLFWwindow* window, double x, double y)
			{
				VulkanWindow* windowClass = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));

				// check if right mouse button is pressed, then move the camera
				if (!Enigma::isDebug)
				{
					double x_offset = x - windowClass->m_lastMousePosX;
					double y_offset = windowClass->m_lastMousePosY - y;

					windowClass->m_lastMousePosX = x;
					windowClass->m_lastMousePosY = y;

					const float mouseSensitivity = 0.1f;
					x_offset *= mouseSensitivity;
					y_offset *= mouseSensitivity;

					windowClass->yaw += x_offset;
					windowClass->pitch += y_offset;

					glm::vec3 direction = glm::vec3(0.0f);
					direction.x = static_cast<float>(std::cos(glm::radians(windowClass->yaw)) * std::cos(glm::radians(windowClass->pitch)));
					direction.y = static_cast<float>(std::sin(glm::radians(windowClass->pitch)));
					direction.z = static_cast<float>(std::sin(glm::radians(windowClass->yaw)) * std::cos(glm::radians(windowClass->pitch)));
					windowClass->camera->SetDirection(glm::normalize(direction));
				}
			}

		public:
			Camera* camera = nullptr;
			GLFWwindow* window = nullptr;
			VkSurfaceKHR surface = VK_NULL_HANDLE;

			uint32_t presentFamilyIndex = 0;
			VkQueue presentQueue = VK_NULL_HANDLE;

			VkSwapchainKHR swapchain = VK_NULL_HANDLE;
			std::vector<VkImage> swapchainImages;
			std::vector<VkImageView> swapchainImageViews;
			std::vector<VkFramebuffer> swapchainFramebuffers;
			VkRenderPass renderPass;

			VkFormat swapchainFormat;
			VkExtent2D swapchainExtent{};
			bool hasResized = false;

			double m_lastMousePosX;
			double m_lastMousePosY;

			double yaw = 30.0f;
			double pitch = 0.0f;

		private:
			const VulkanContext& context;
	};

	VulkanWindow PrepareWindow(uint32_t width, uint32_t height, VulkanContext& context);
	void MakeVulkanWindow(VulkanWindow& windowContext, VulkanContext& context, Camera* camera);
	void RecreateSwapchain(const VulkanContext& context, VulkanWindow& window);
	void TearDownSwapchain(const VulkanContext& context, VulkanWindow& window);
}