

#include <iostream>
#include <stdlib.h>
#include <windows.h>
#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Core/VulkanWindow.h"
#include "Graphics/VulkanContext.h"
#include "Graphics/Renderer.h"
#include "Graphics/Player.h"

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

    // Set cursor and keyboard callbacks
    glfwSetKeyCallback(window.window, window.glfw_callback_key_press);
    glfwSetCursorPosCallback(window.window, window.glfw_callback_mouse);

    // Create a directional light: Position, colour and intensity ( perhaps this should be moved to the Renderer )
    Enigma::WorldInst.Lights.push_back(Enigma::CreateDirectionalLight(glm::vec4(0.0f, 10.0f, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.0));

    Enigma::Renderer renderer = Enigma::Renderer(context, window, &FPSCamera);

    Enigma::WorldInst.Characters.push_back(Enigma::WorldInst.player);
    for (int i = 0; i < Enigma::WorldInst.Enemies.size(); i++) {
        Enigma::WorldInst.Characters.push_back(Enigma::WorldInst.Enemies[i]);
    }

    // this is unsafe, 
    // what if index 0 is invalid? you'll crash the entire program. probably should check just in case before doing this
    for (int i = 0; i < Enigma::WorldInst.Characters.size(); i++) {
        Enigma::WorldInst.Enemies[0]->addToNavmesh(Enigma::WorldInst.Characters[i], Enigma::WorldInst.Meshes[0]);
    }

    // game loop: to keep updating and rendering the game
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
    delete Enigma::WorldInst.player;
    return 0;
}