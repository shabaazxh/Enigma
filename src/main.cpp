

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
#include "Core/Engine.h"


int main() {

    Enigma::Time* timer = new Enigma::Time();
    Enigma::Camera FPSCamera = Enigma::Camera(glm::vec3(-16.0f, 6.1f, -3.07), glm::normalize(glm::vec3(30, 0, -1) + glm::vec3(0, 0, 0)), glm::vec3(0, 1, 0), *timer, 800.0f / 600.0f);
    
    // Prepare context initializes Volk and prepares a instance with debug enabled (if in DEBUG mode).
    Enigma::VulkanContext context = Enigma::PrepareContext();
    // Prepare window initializes glfw and creates a VkSurfaceKHR. A valid surface will allow us to query for a present queue
    Enigma::VulkanWindow  window  = Enigma::PrepareWindow(1920, 1080, context);

    // Finialize the context by selecting a gpu, creating device
    Enigma::MakeVulkanContext(context, window.surface);
    // Finialize the window by creating swapchain resources for presentation
    Enigma::MakeVulkanWindow(window, context, &FPSCamera);

    glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(window.window, &window);

    Enigma::Renderer renderer = Enigma::Renderer(context, window, &FPSCamera);

    glfwSetKeyCallback(window.window, window.glfw_callback_key_press);
    glfwSetCursorPosCallback(window.window, window.glfw_callback_mouse);


    while (!glfwWindowShouldClose(window.window)) {
        timer->Update();
        FPSCamera.Update(window.swapchainExtent.width, window.swapchainExtent.height);
        renderer.Update(&FPSCamera);
        renderer.DrawScene();
        glfwPollEvents();
    }

    glfwDestroyWindow(window.window);

    glfwTerminate();

    delete timer;
    return 0;
}