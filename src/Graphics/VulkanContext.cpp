#include "VulkanContext.h"

#include <vector>
#include <optional>
#include <utility>
#include <cstdio>
#include <unordered_set>
#include <cassert>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace
{
	std::unordered_set<std::string> GetInstanceLayers();
	std::unordered_set<std::string> GetInstanceExtensions();

	VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance);
	VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_callback(VkDebugUtilsMessageSeverityFlagBitsEXT aSeverity, VkDebugUtilsMessageTypeFlagsEXT aType, VkDebugUtilsMessengerCallbackDataEXT const* aData, void*);

}

//VkInstance instance = VK_NULL_HANDLE;
//VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
//VkDevice device = VK_NULL_HANDLE;
//
//uint32_t graphicsFamilyIndex = 0;
//VkQueue graphicsQueue = VK_NULL_HANDLE;
//VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
namespace Enigma
{
	VulkanContext::VulkanContext() = default;

	VulkanContext::VulkanContext(VulkanContext&& other) noexcept :
		instance(std::exchange(other.instance, VK_NULL_HANDLE)),
		physicalDevice(std::exchange(other.physicalDevice, VK_NULL_HANDLE)),
		device(std::exchange(other.device, VK_NULL_HANDLE)),
		graphicsFamilyIndex(std::exchange(other.graphicsFamilyIndex, 0)),
		graphicsQueue(std::exchange(other.graphicsQueue, VK_NULL_HANDLE)),
		debugMessenger(std::exchange(other.debugMessenger, VK_NULL_HANDLE))
		 {}

	VulkanContext& VulkanContext::operator=(VulkanContext&& other) noexcept
	{
		std::swap(instance, other.instance);
		std::swap(physicalDevice, other.physicalDevice);
		std::swap(device, other.device);
		std::swap(graphicsFamilyIndex, other.graphicsFamilyIndex);
		std::swap(graphicsQueue, other.graphicsQueue);
		std::swap(debugMessenger, other.debugMessenger);

		return *this;
	}

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

	float ScoreDevice(VkPhysicalDevice pDevice)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(pDevice, &props);

		// consider devices which support at least Vulkan 1.2 
		const auto major = VK_API_VERSION_MAJOR(props.apiVersion);
		const auto minor = VK_API_VERSION_MINOR(props.apiVersion);

		if (major < 1 || (major == 1 && minor < 2))
		{
			return -1.0f;
		}

		float score = 0.f;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(pDevice, nullptr, &extensionCount, availableExtensions.data());

		VkPhysicalDeviceFeatures2 features{};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		
		vkGetPhysicalDeviceFeatures2(pDevice, &features);


		// prefer to select and use a discrete GPU over an integrated one 
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.features.samplerAnisotropy)
		{
			score += 500.0f;
		}
		else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			score += 100.0f;
		}

		return score;
	}

	VkPhysicalDevice SelectDevice(VkInstance instance)
	{
		uint32_t numberDevices = 0;
		vkEnumeratePhysicalDevices(instance, &numberDevices, nullptr);

		std::vector<VkPhysicalDevice> pDevices(numberDevices);
		vkEnumeratePhysicalDevices(instance, &numberDevices, pDevices.data());

		// score the device depending on the features we would be wanting to use
		float bestScore = -1.0f;
		VkPhysicalDevice pDevice = VK_NULL_HANDLE;

		// From the available devices, find the one we want
		// we score them based on our preferences
		for (const auto& device : pDevices)
		{
			float score = ScoreDevice(device);
			if (score > bestScore)
			{
				bestScore = score;
				pDevice = device;
			}
		}


		VkPhysicalDeviceFeatures2 features{};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		vkGetPhysicalDeviceFeatures2(pDevice, &features);

		std::printf("Anisotropic support: %s\n", &features.features.samplerAnisotropy ? "TRUE" : "FALSE");

		return pDevice;
	}

	std::optional<uint32_t> FindGraphicsQueueFamily(VkPhysicalDevice pDevice)
	{
		uint32_t numQueues = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &numQueues, nullptr);

		std::vector<VkQueueFamilyProperties> families(numQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &numQueues, nullptr);

		for (uint32_t i = 0; i < numQueues; i++)
		{
			const auto& family = families[i];

			// check if this queue family is graphics
			if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				return i;
			}
		}

		return {};
	}

	VkDevice CreateDevice(VkPhysicalDevice pDevice, uint32_t graphicsFamilyIndex)
	{
		float queuePriorities[1] = { 1.f };

		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = graphicsFamilyIndex;
		queueInfo.pQueuePriorities = queuePriorities;
		queueInfo.queueCount = 1;

		VkPhysicalDeviceFeatures selectecdFeatures{};
		selectecdFeatures.samplerAnisotropy = VK_TRUE;

		std::vector<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo deviceInfo{};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.pQueueCreateInfos = &queueInfo;
		deviceInfo.pEnabledFeatures = &selectecdFeatures;
		deviceInfo.enabledExtensionCount = uint32_t(extensions.size());
		deviceInfo.ppEnabledExtensionNames = extensions.data();

		VkDevice device = VK_NULL_HANDLE;

		VK_CHECK(vkCreateDevice(pDevice, &deviceInfo, nullptr, &device));
		return device;
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

		bool enabledDebugUtils = true;

# if !defined(NDEBUG) // DEBUG BUILD LAYERS AND EXTENSIONS
		if (supportedLayers.count("VK_LAYER_KHRONOS_validation"))
		{
			enabledLayers.emplace_back("VK_LAYER_KHRONOS_validation");
		}

		if (supportedExtensions.count("VK_EXT_debug_utils"))
		{
			enabledDebugUtils = true;
			enabledExtensions.emplace_back("VK_EXT_debug_utils");
		}
#endif

		glfwInit();

		// cannot do this before the window is created 
		uint32_t reqExtCount = 0;
		const char** requiredExt = glfwGetRequiredInstanceExtensions(&reqExtCount);

		for (uint32_t i = 0; i < reqExtCount; i++)
		{
			if (!supportedExtensions.count(requiredExt[i]))
			{
				std::runtime_error("GLFW/Vulkan: required instance extension not supported");
			}

			enabledExtensions.emplace_back(requiredExt[i]);
		}

		// output the enabled layers and extensions
		for (const auto& layer : enabledLayers)
			std::fprintf(stderr, "Enabling layer: %s\n", layer);
		for (const auto& extension : enabledExtensions)
			std::fprintf(stderr, "Enabling instance extensions: %s\n", extension);


		// init the vulkan instance
		context.instance = CreateVulkanInstance(enabledLayers, enabledExtensions, enabledDebugUtils);

		volkLoadInstance(context.instance);

		// debug messenger set up 
		if (enabledDebugUtils)
			context.debugMessenger = CreateDebugMessenger(context.instance);

		context.physicalDevice = SelectDevice(context.instance);

		if (context.physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to find suitable device!");
		}

		// If we have a suitable device, display it in the output console
		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(context.physicalDevice, &props);

			std::fprintf(stderr, "Selected device: %s\n", props.deviceName);
		}

		// With the physical device, we can now create the logical device
		// to do this, we need the graphics family index since we will be doing graphics operations


		const auto index = FindGraphicsQueueFamily(context.physicalDevice);

		if (index.has_value())
			context.graphicsFamilyIndex = *index;

		context.device = CreateDevice(context.physicalDevice, context.graphicsFamilyIndex);


		// retrieve the vkqueue 
		vkGetDeviceQueue(context.device, context.graphicsFamilyIndex, 0, &context.graphicsQueue);

		assert(context.graphicsQueue != VK_NULL_HANDLE);

		return context;
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

	VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		debugInfo.pfnUserCallback = &debug_util_callback;
		debugInfo.pUserData = nullptr;

		VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
		VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &debugInfo, nullptr, &messenger));

		return messenger;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_callback(VkDebugUtilsMessageSeverityFlagBitsEXT aSeverity, VkDebugUtilsMessageTypeFlagsEXT aType, VkDebugUtilsMessengerCallbackDataEXT const* aData, void*)
	{
		std::cerr << "validation layer: " << aData->pMessage << std::endl;

		return VK_FALSE;
	}
}
