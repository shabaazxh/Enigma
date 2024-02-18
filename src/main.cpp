

#include <iostream>
#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Core/VulkanWindow.h"
#include "Graphics/VulkanContext.h"

int main() {

    Enigma::VulkanContext context = Enigma::MakeVulkanContext();
    Enigma::VulkanWindow window = Enigma::MakeVulkanWindow(context);


    while (!glfwWindowShouldClose(window.window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window.window);

    glfwTerminate();

    return 0;
}