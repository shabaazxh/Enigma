

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

    Enigma::Model* temp = new Enigma::Model("../resources/level1.obj", context, ENIGMA_LOAD_OBJ_FILE);
    Enigma::WorldInst.Meshes.push_back(temp);

    //add enemy to the world class
    Enigma::WorldInst.Enemies.push_back(new Enigma::Enemy("../resources/zombie-walk-test/source/Zombie_Walk1.fbx", context, ENIGMA_LOAD_FBX_FILE, glm::vec3(60.f, 0.1f, 0.f), glm::vec3(0.1f, 0.1f, 0.1f), 0, 180, 0));
    Enigma::WorldInst.Enemies.push_back(new Enigma::Enemy("../resources/zombie-walk-test/source/Zombie_Walk1.fbx", context, ENIGMA_LOAD_FBX_FILE, glm::vec3(60.f, 0.1f, 50.f), glm::vec3(0.1f, 0.1f, 0.1f), 0, 180, 0));

    //add player to world class
    Enigma::WorldInst.player = new Enigma::Player("../resources/player.fbx", context, ENIGMA_LOAD_FBX_FILE, glm::vec3(-100.f, 0.1f, -40.f), glm::vec3(0.1f, 0.1f, 0.09f), 90, 0, 0);
    Enigma::WorldInst.player->addEquipment(new Enigma::Equipment(glm::vec3(0.f, 13.f, 0.f), new Enigma::Model("../resources/gun.obj", context, ENIGMA_LOAD_OBJ_FILE)));

    Enigma::WorldInst.addMeshesToWorld(Enigma::WorldInst.player, Enigma::WorldInst.Enemies);

    //add the player and enemies to the correct query lists
    Enigma::WorldInst.addCharactersToWorld(Enigma::WorldInst.player, Enigma::WorldInst.Enemies);
    Enigma::WorldInst.addCharctersToNavmesh(Enigma::WorldInst.Characters);

    glfwSetKeyCallback(window.window, window.glfw_callback_key_press);
    glfwSetCursorPosCallback(window.window, window.glfw_callback_mouse);


    while (!glfwWindowShouldClose(window.window)) {
        Enigma::WorldInst.ManageAIs(Enigma::WorldInst.Characters, Enigma::WorldInst.Meshes[0], Enigma::WorldInst.player, Enigma::WorldInst.Enemies);
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