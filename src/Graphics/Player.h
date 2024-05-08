#pragma once

#include "../Graphics/Character.h"
#include "../Core/Camera.h"

namespace Enigma
{
	class Player : public Character
	{
		public:
			Player(const std::string& filepath, const VulkanContext& context, int filetype);
			Player(const std::string& filepath, const VulkanContext& context, int filetype, glm::vec3 trans, glm::vec3 scale);
			Player(const std::string& filepath, const VulkanContext& context, int filetype, glm::vec3 trans, glm::vec3 scale, float x, float y, float z);
			Player(const std::string& filepath, const VulkanContext& context, int filetype, glm::vec3 trans, glm::vec3 scale, glm::mat4 rm);
			Player(const VulkanContext& context);

			void ManageInputs();
	};
}

