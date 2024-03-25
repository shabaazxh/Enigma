#pragma once

#include "../Graphics/Model.h"

#define NO_MODEL "none"

namespace Enigma
{
	class Character
	{
		public:
			float health;
			Model* model;
			bool noModel = false;
			std::vector<std::string> equipment;
			glm::vec3 translation = glm::vec3(0.f, 0.f, 0.f);
			float rotationX = 0.f;
			float rotationY = 0.f;
			float rotationZ = 0.f;
			glm::mat4 rotMatrix = glm::mat4(1.0f);
			glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);

			Character(const std::string& filepath, const VulkanContext& context, int filetype);

			void ManageAnimation();
			void ManageCollisions();
			void setTranslation(glm::vec3 trans);
			void setScale(glm::vec3 scale);
			void setRotationX(float angle);
			void setRotationY(float angle);
			void setRotationZ(float angle);
			void setRotationMatrix(glm::mat4 rotMatrix);
	};
}