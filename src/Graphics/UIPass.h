#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "../Core/VulkanWindow.h"
#include "Common.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

#define VERTEX_HEAD    "../resources/Shaders/head.vert.spv"
#define FRAGMENT_HEAD  "../resources/Shaders/head.frag.spv"
#define VERTEX_BLOOD   "../resources/Shaders/blood.vert.spv"
#define FRAGMENT_BLOOD "../resources/Shaders/blood.frag.spv"

namespace Enigma {

class UIPass {
public:
    UIPass(const VulkanContext& context, const VulkanWindow& window);
    ~UIPass();

    void Execute(VkCommandBuffer cmd);
    void Update();
    void Resize(VulkanWindow& window);

private:
    // void CreatePipeline(VkDevice device, VkExtent2D swapchainExtent);
    // void BuildDescriptorSetLayout(const VulkanContext& context);
    void CreateRenderPass();

    void CreateBlood();
    void UpdateBlood(float value);
    void DrawBlood(VkCommandBuffer cmdBuffer);

    void CreateBloodPipeline();
    void CreateBloodVertexBuffer();
    void CreateHeadPipeline();
    void CreateHeadImage();

private:
    const VulkanContext& context;
    const VulkanWindow& window;
    uint32_t m_width;
    uint32_t m_height;

    VkRenderPass m_RenderPass;

    // blood
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
};

}  // namespace Enigma