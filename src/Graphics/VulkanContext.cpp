#include "VulkanContext.h"

#include <vector>
#include <optional>
#include <utility>
#include <cstdio>
#include <unordered_set>

namespace
{
	std::unordered_set<std::string> GetInstanceLayers();
	std::unordered_set<std::string> GetInstanceExtensions();

	VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_callback(VkDebugUtilsMessageSeverityFlagBitsEXT aSeverity, VkDebugUtilsMessageTypeFlagsEXT aType, VkDebugUtilsMessengerCallbackDataEXT const* aData, void*);
}

namespace Enigma
{
	VulkanContext::VulkanContext() = default;

	VulkanContext::~VulkanContext()
	{
		if (device != VK_NULL_HANDLE)
			vkDestroyDevice(device, nullptr);
		
		if (debugMessenger != VK_NULL_HANDLE)
			vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		
		if (instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(instance, nullptr);
		}
	}

	VulkanContext MakeVulkanContext()
	{
		VulkanContext context;

		VK_CHECK(volkInitialize());

		// get the instance layer and extensions
		const auto supportedLayers = GetInstanceLayers();
		const auto supportedExtensions = GetInstanceExtensions();

		std::vector<const char*> enabledLayers;
		std::vector<const char*> enabledExtensions;

# if !defined(NDEBUG) // DEBUG BUILD LAYERS AND EXTENSIONS
		if (supportedLayers.count("VK_LAYER_KHORNOS_validation"))
		{
			enabledLayers.emplace_back("VK_LAYER_KHORNOS_validation");
		}

		if (supportedExtensions.count("VK_EXT_debug_utils"))
		{
			enabledExtensions.emplace_back("VK_EXT_debug_utils");
		}
#endif

		// output the enabled layers and extensions
		for (const auto& layer : enabledLayers)
			std::fprintf(stderr, "Enabling layer: %s\n", layer);
		for (const auto& extension : enabledExtensions)
			std::fprintf(stderr, "Enabling instance extensions: %s\n", extension);


		// init the vulkan instance
		//context.instance = CreateInstance();


	}

	VkInstance CreateVulkanInstance(const std::vector<const char*>& enabledLayers, const std::vector<const char*>& enabledExtensions, bool enabledDebugUtils)
	{
		// if debug is enabled then prepare the debug messenger info

		VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
		if (enabledDebugUtils)
		{
			debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugInfo.pfnUserCallback = &debug_util_callback;
			debugInfo.pUserData = nullptr;

		}

		// Set up the application information 
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Enigma";
		appInfo.applicationVersion = 2024;
		appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0); // version 1.3

		// Set up the instance 
		VkInstanceCreateInfo instanceInfo{};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		
		// provide instance layers
		instanceInfo.enabledLayerCount = uint32_t(enabledLayers.size());
		instanceInfo.ppEnabledLayerNames = enabledLayers.data();

		// provide instance extensions
		instanceInfo.enabledExtensionCount = uint32_t(enabledExtensions.size());
		instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();

		// provide the application info we set up earlier 
		instanceInfo.pApplicationInfo = &appInfo;

		if (enabledDebugUtils)
		{
			debugInfo.pNext = instanceInfo.pNext;
			instanceInfo.pNext = &debugInfo;
		}

		VkInstance instance = VK_NULL_HANDLE;
		VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));

		return instance;
	}
}

namespace
{
	std::unordered_set<std::string> GetInstanceLayers()
	{
		// Query for the number of layers
		uint32_t numLayers = 0;
		vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

		// Store the layers
		std::vector<VkLayerProperties> layers(numLayers);
		vkEnumerateInstanceLayerProperties(&numLayers, layers.data());

		std::unordered_set<std::string> res;
		for (const auto& layer : layers)
			res.insert(layer.layerName);

		return res;
	}

	std::unordered_set<std::string> GetInstanceExtensions()
	{
		uint32_t numExtesnions = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &numExtesnions, nullptr);

		std::vector<VkExtensionProperties> extensions(numExtesnions);
		vkEnumerateInstanceExtensionProperties(nullptr, &numExtesnions, extensions.data());

		std::unordered_set<std::string> res;
		for (const auto& extension : extensions)
			res.insert(extension.extensionName);

		return res;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_callback(VkDebugUtilsMessageSeverityFlagBitsEXT aSeverity, VkDebugUtilsMessageTypeFlagsEXT aType, VkDebugUtilsMessengerCallbackDataEXT const* aData, void*)
	{
		std::cerr << "validation layer: " << aData->pMessage << std::endl;

		return VK_FALSE;
	}
}
