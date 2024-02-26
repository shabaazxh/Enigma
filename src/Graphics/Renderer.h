#pragma once
#define _USE_MATH_DEFINES
#include <Volk/volk.h>
#include<glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Allocator.h"
#include "VulkanObjects.h"
#include "VulkanContext.h"
#include "Model.h"
#include "../Core/VulkanWindow.h"

#include <vector>

namespace glsl
{
	struct SceneUniform {
		// Note: need to be careful about the packing/alignment here!
		glm::mat4 camera;
		glm::mat4 projection;
		glm::mat4 projCam;
	};

	static_assert(sizeof(SceneUniform) <= 65536, "SceneUniform must be less than 65536 bytes for vkCmdUpdateBuffer");
	static_assert(sizeof(SceneUniform) % 4 == 0, "SceneUniform size must be a multiple of 4 bytes");
}

namespace Enigma
{
	inline uint32_t currentFrame = 0;
	inline uint32_t max_frames_in_flight = 3;

	struct UserState
	{
		//bool inputMap[std::size_t(EInputState::max)] = {};

		float mouseX = 0.f, mouseY = 0.f;
		float previousX = 0.f, previousY = 0.f;

		bool wasMousing = false;

		glm::mat4 camera2world = glm::identity<glm::mat4>();
	};

	class Renderer
	{
		public:
			explicit Renderer(const VulkanContext& context, VulkanWindow& window);
			~Renderer();
			void DrawScene();

			void CreateGraphicsPipeline();

			UserState state;
			glsl::SceneUniform sceneUniform{};
			Allocator allocator;
		private:
			void CreateRendererResources();
			void update_scene_uniforms(glsl::SceneUniform&, std::uint32_t aFramebufferWidth, std::uint32_t aFramebufferHeight, UserState const& aState);
		private:
			const VulkanContext& context;
			VulkanWindow& window;

			std::vector<Fence> m_fences;
			std::vector<Semaphore> m_imagAvailableSemaphores;
			std::vector<Semaphore> m_renderFinishedSemaphores;

			std::vector<CommandPool> m_renderCommandPools;
			std::vector<VkCommandBuffer> m_renderCommandBuffers;

			Pipeline m_pipeline;
			PipelineLayout m_pipelineLayout;
			std::vector<Model*> m_renderables;
	};
}