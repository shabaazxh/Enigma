

#include <iostream>
#include <stdlib.h>
#include <windows.h>
#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Core/VulkanWindow.h"
#include "Graphics/VulkanContext.h"
#include "Graphics/Renderer.h"
#include "Graphics/Player.h"

int main() {

    Enigma::EngineTime = new Enigma::Time();
    Enigma::Camera FPSCamera = Enigma::Camera(glm::vec3(-16.0f, 6.1f, -3.07), glm::normalize(glm::vec3(-16.0f, 6.1f, -3.07) + glm::vec3(0, 0, -1)), glm::vec3(0, 1, 0), *Enigma::EngineTime, 1920.0f / 1080.0f);
    
    // Prepare context initializes Volk and prepares a instance with debug enabled (if in DEBUG mode).
    Enigma::VulkanContext context = Enigma::PrepareContext();
    // Prepare window initializes glfw and creates a VkSurfaceKHR. A valid surface will allow us to query for a present queue
    Enigma::VulkanWindow  window  = Enigma::PrepareWindow(1920, 1080, context);

    // Finialize the context by selecting a gpu, creating device
    Enigma::MakeVulkanContext(context, window.surface);
    // Finialize the window by creating swapchain resources for presentation
    Enigma::MakeVulkanWindow(window, context, &FPSCamera);

    glfwSetWindowUserPointer(window.window, &window);

    // Set cursor and keyboard callbacks
    glfwSetKeyCallback(window.window, window.glfw_callback_key_press);
    glfwSetCursorPosCallback(window.window, window.glfw_callback_mouse);

    Enigma::Renderer renderer = Enigma::Renderer(context, window, &FPSCamera);

    // Create a directional light: Position, colour and intensity -125.242f, 359.0f, -67.708, 1.0f
    Enigma::Light SunLight = Enigma::CreateDirectionalLight(glm::vec4(-45.802f, 105.0f, 23.894, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.0);
    Enigma::WorldInst.Lights.push_back(SunLight);

    Enigma::Model* obj1 = new Enigma::Model("../resources/level1.obj", context, ENIGMA_LOAD_OBJ_FILE);
    Enigma::Model* LightBulb = new Enigma::Model("../resources/Light/Light.obj", context, ENIGMA_LOAD_OBJ_FILE, "Light");
    Enigma::WorldInst.Meshes.push_back(obj1);
    Enigma::WorldInst.Meshes.push_back(LightBulb);

    //add enemy to the world class
    Enigma::WorldInst.Enemies.push_back(new Enigma::Enemy("../resources/zombie-walk-test/source/Zombie_Walk.fbx", context, ENIGMA_LOAD_FBX_FILE, glm::vec3(60.f, 0.1f, 0.f), glm::vec3(0.1f, 0.1f, 0.1f), 0, 0, 0));
    Enigma::WorldInst.Enemies.push_back(new Enigma::Enemy("../resources/zombie-walk-test/source/Zombie_Walk.fbx", context, ENIGMA_LOAD_FBX_FILE, glm::vec3(60.f, 0.1f, 50.f), glm::vec3(0.1f, 0.1f, 0.1f), 0, 0, 0));

    //world.Enemies[0]->model->updateAnimation(world.Enemies[0]->model->m_Scene, timer->deltaTime);

    //add player to world class
    Enigma::WorldInst.player = new Enigma::Player(context, "../resources/Weapon/g.obj", glm::vec3(-100.0f, 100.0f, -40.0f), 100, *Enigma::EngineTime);
    Enigma::WorldInst.player->setScale(glm::vec3(0.2f, 0.2f, 0.2f));

    Enigma::WorldInst.addMeshesToWorld(Enigma::WorldInst.player, Enigma::WorldInst.Enemies);

    ////add the player and enemies to the correct query lists
    Enigma::WorldInst.addCharactersToWorld(Enigma::WorldInst.player, Enigma::WorldInst.Enemies);

    Enigma::WorldInst.addCharctersToNavmesh(Enigma::WorldInst.Characters);

    // game loop: to keep updating and rendering the game
    while (!glfwWindowShouldClose(window.window)) {
        Enigma::WorldInst.ManageAIs(Enigma::WorldInst.Characters, obj1, Enigma::WorldInst.player, Enigma::WorldInst.Enemies, Enigma::EngineTime);
        Enigma::EngineTime->Update();
        FPSCamera.Update(window.swapchainExtent.width, window.swapchainExtent.height);
        renderer.Update(&FPSCamera);
        renderer.DrawScene();
        glfwPollEvents();
    }

    glfwDestroyWindow(window.window);
    glfwTerminate();

    delete Enigma::EngineTime;
    delete Enigma::WorldInst.player;
    return 0;
}