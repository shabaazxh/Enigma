#pragma once

#include <Volk/volk.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "../Core/Error.h"
#include "Allocator.h"
#include <string>
#include <array>
#include <vector>
#include <iostream>
#include "../Graphics/VulkanImage.h"
#include "../Graphics/Model.h"

namespace Enigma
{
	class Character : public Model
	{
		public:
			float health;
			std::vector<std::string> equipment;

			void ManageAnimation();
			void ManageCollisions();
	};
}