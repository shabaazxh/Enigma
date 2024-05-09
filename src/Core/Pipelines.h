#include <Volk/volk.h>
#include "../Graphics/VulkanBuffer.h"
#include <Volk/volk.h>
#include "../Graphics/Allocator.h"
#include "../Graphics/VulkanObjects.h"
#include "../Graphics/VulkanContext.h"
#include "../Graphics/Common.h"
#include "../Core/VulkanWindow.h"
#include <memory.h>
#include <vector>
#include "../Core/Error.h"
#include "../Core/Engine.h"
#include "../Core/World.h"

namespace Enigma
{
	Pipeline CreateGraphicsPipeline(const std::string& vertex, const std::string& fragment, VkBool32 enableBlend, VkBool32 enableDepth, VkBool32 enableDepthWrite, const std::vector<VkDescriptorSetLayout>& descriptorLayouts, PipelineLayout& pipelinelayout, VkPrimitiveTopology topology, const VulkanContext& context, VulkanWindow& window);
}