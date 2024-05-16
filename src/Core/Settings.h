#pragma once

#include <Volk/volk.h>
#include "../Core/Error.h"
#include "../Graphics/VulkanContext.h"
#include "../Graphics/VulkanObjects.h"
#include <GLFW/glfw3.h>
#include "../Graphics/Light.h"
#include <fstream>
#include "../Core/Error.h"
#include <string>
#include "../Graphics/Model.h"

namespace Enigma
{
	inline bool isDebug = true;
	inline bool enablePlayerCamera = false;
	inline std::vector<Model*> tempModels;
	inline bool renderTemp = true;

	inline float translationAmplitude = 1.0f; // Adjust as needed
	inline float translationFrequency = 1.0f; // Adjust as needed
	inline float rotationAmplitude = 10.0f;     // Adjust as needed
	inline float rotationFrequency = 0.5f;     // Adjust as needed
}