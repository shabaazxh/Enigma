#pragma once
#include <Volk/volk.h>
#include <iostream>


#define ENIGMA_VK_CHECK(call, message) \
do { \
    VkResult result = call; \
    if (result != VK_SUCCESS) { \
        std::cout << "[ENIGMA ERROR]: " << message << " VkResult: " << result << std::endl; \
    } \
} while (0)

#define ENIGMA_ERROR(message) std::cout << "[ENIGMA ERROR]: " << message << std::endl; \
