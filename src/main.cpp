

#include <iostream>
#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Core/VulkanWindow.h"
#include "Graphics/VulkanContext.h"
#include "Graphics/Renderer.h"
#include "Graphics/Model.h"

int main() {

    Enigma::VulkanContext context = Enigma::MakeVulkanContext();
    Enigma::VulkanWindow window = Enigma::MakeVulkanWindow(context);

    Enigma::Renderer renderer = Enigma::Renderer(context, window);

    while (!glfwWindowShouldClose(window.window)) {

        renderer.DrawScene();
        glfwPollEvents();
    }

    glfwDestroyWindow(window.window);

    glfwTerminate();

    return 0;
}