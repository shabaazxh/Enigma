#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "Renderer.h"

namespace Enigma
{
	class UI
	{
		public:
        void CreateBloodPipeline();
        void CreateBloodVertexBuffer();
        void CreateHeadPipeline();
        void CreateHeadImage();
        void CreateBlood();
        void UpdateBlood(float value);
        void DrawBlood(VkCommandBuffer cmdBuffer);

        
       private:
           const VulkanContext &context;
           VulkanWindow &window;
           std::vector<glm::vec2> bloodVertices;
           Buffer bloodVertexBuffer;
           glm::mat4 bloodTransformMatrix;
           Pipeline m_bloodPipeline;
           Pipeline m_bloodPipeline2;
           PipelineLayout m_bloodPipelineLayout;

           Image m_headImage;
           VkDescriptorSetLayout m_headDescriptorSetLayout;
           VkDescriptorSet m_headDescriptorSet;
           glm::mat4 m_headTransformMatrix;
           PipelineLayout m_headPipelineLayout;
           Pipeline m_headPipeline;

           Image m_crosshairImage;
           VkDescriptorSetLayout m_crosshairDescriptorSetLayout;
           VkDescriptorSet m_crosshairDescriptorSet;
           glm::mat4 m_crosshairTransformMatrix;
           PipelineLayout m_crosshairPipelineLayout;
           Pipeline m_crosshairPipeline;
        
	};}


