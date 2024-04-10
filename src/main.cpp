

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

    //initalize world struct
    Enigma::World world;

    Enigma::Renderer renderer = Enigma::Renderer(context, window, &FPSCamera, &world);

    //ToDo: Max this just one function between character class
    //add enemy to the world class
    Enigma::Model* temp = new Enigma::Model("../resources/level1.obj", context, ENIGMA_LOAD_OBJ_FILE);
    world.Meshes.push_back(temp);
    Enigma::Enemy* enemy1 = new Enigma::Enemy("../resources/zombie-walk-test/source/Zombie_Walk1.fbx", context, ENIGMA_LOAD_FBX_FILE);
    world.Meshes.push_back(enemy1->model);
    enemy1->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
    enemy1->setTranslation(glm::vec3(60.f, 0.1f, 0.f));
    world.Enemies.push_back(enemy1);

    //add enemy to the world class
    Enigma::Enemy* enemy2 = new Enigma::Enemy("../resources/zombie-walk-test/source/Zombie_Walk1.fbx", context, ENIGMA_LOAD_FBX_FILE);
    world.Meshes.push_back(enemy2->model);
    enemy2->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
    enemy2->setTranslation(glm::vec3(60.f, 0.1f, 50.f));
    world.Enemies.push_back(enemy2);

    //add player to world class
    world.player = new Enigma::Player("../resources/gun.obj", context, ENIGMA_LOAD_OBJ_FILE);
    if (!world.player->noModel) {
        world.Meshes.push_back(world.player->model);
    }
    world.player->setTranslation(glm::vec3(-100.f, 0.1f, -40.f));
    world.player->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
    world.player->setRotationY(180);

    //add the player and enemies to the correct query lists
    world.Characters.push_back(world.player);
    for (int i = 0; i < world.Enemies.size(); i++) {
        world.Characters.push_back(world.Enemies[i]);
    }

    for (int i = 0; i < world.Characters.size(); i++) {
       world.Enemies[0]->addToNavmesh(world.Characters[i], world.Meshes[0]);
    }

    //Sleep(1000);

    glfwSetKeyCallback(window.window, window.glfw_callback_key_press);
    glfwSetCursorPosCallback(window.window, window.glfw_callback_mouse);


    while (!glfwWindowShouldClose(window.window)) {
        world.ManageAIs(world.Characters, world.Meshes[0], world.player, world.Enemies);
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