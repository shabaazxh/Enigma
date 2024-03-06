

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

    Enigma::Time* timer = new Enigma::Time();
    Enigma::Camera FPSCamera = Enigma::Camera(glm::vec3(-31.0f, 6.1f, -3.07), glm::normalize(glm::vec3(30, 0, -1) + glm::vec3(0, 0, 0)), glm::vec3(0, 1, 0), *timer, 800.0f / 600.0f);
    
    Enigma::VulkanContext context = Enigma::MakeVulkanContext();
    Enigma::Allocator allocator = Enigma::MakeAllocator(context);
    Enigma::VulkanWindow window = Enigma::MakeVulkanWindow(1920, 1080, context, &FPSCamera, allocator);

    glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(window.window, &window);

    Enigma::Renderer renderer = Enigma::Renderer(context, window, &FPSCamera, allocator);

    glfwSetKeyCallback(window.window, window.glfw_callback_key_press);
    glfwSetCursorPosCallback(window.window, window.glfw_callback_mouse);

    while (!glfwWindowShouldClose(window.window)) {

        timer->Update();
        FPSCamera.Update(window.swapchainExtent.width, window.swapchainExtent.height);
        renderer.DrawScene();

        auto pos = FPSCamera.GetPosition();

        //std::cout << "Camera: " << pos.x << ", " << pos.y << " , " << pos.z << std::endl;
        glfwPollEvents();
    }

    glfwDestroyWindow(window.window);

    glfwTerminate();

    delete timer;
    // -31, 6.1, -3.07
    return 0;
}