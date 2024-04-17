#pragma once
#include <Volk/volk.h>
#include "Allocator.h"
#include "VulkanObjects.h"
#include "VulkanContext.h"
#include "Model.h"
#include "Common.h"
#include "../Core/VulkanWindow.h"
#include <memory.h>
#include <vector>
#include "../Core/Error.h"
#include "../Core/Engine.h"
#include "../Core/World.h"

#include <memory>
#include "GBuffer.h"
#include "Lighting.h"
#include "Composite.h"

namespace Enigma
{
	class Renderer
	{
		public:
			explicit Renderer(const VulkanContext& context, VulkanWindow& window, Camera* camera);
			~Renderer();
			void DrawScene();
			void Update(Camera* cam);
		private:
			void CreateRendererResources();
			void CreateDescriptorPool();
		private:
			// vulkan and window context
			const VulkanContext& context;
			VulkanWindow& window;

			// Rendering passes
			GBuffer* m_gBufferPass;
			Lighting* m_lightingPass;
			Composite* m_compositePass;

			// Rendering resources
			std::vector<Fence> m_fences;
			std::vector<Semaphore> m_imagAvailableSemaphores;
			std::vector<Semaphore> m_renderFinishedSemaphores;

			std::vector<CommandPool> m_renderCommandPools;
			std::vector<VkCommandBuffer> m_renderCommandBuffers;

			// other 
			bool current_state = false;
			Camera* camera;
			GBufferTargets gBufferTargets;
	};
}