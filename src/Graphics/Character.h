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
			bool moved = true;
			int navmeshPosition;

		private:
			glm::vec3 translation = glm::vec3(0.f, 0.f, 0.f);
			float rotationX = 0.f;
			float rotationY = 0.f;
			float rotationZ = 0.f;
			glm::mat4 rotMatrix = glm::mat4(1.0f);
			glm::vec3 scale = glm::vec3(1.f, 1.f, 1.f);

		public:	
			Character(const std::string& filepath, const VulkanContext& context, int filetype);
			Character(const std::string& filepath, const VulkanContext& context, int filetype, glm::vec3 trans, glm::vec3 scale);
			Character(const std::string& filepath, const VulkanContext& context, int filetype, glm::vec3 trans, glm::vec3 scale, float x, float y, float z);
			Character(const std::string& filepath, const VulkanContext& context, int filetype, glm::vec3 trans, glm::vec3 scale, glm::mat4 rm);

			void ManageAnimation();
			void ManageCollisions();
			void setTranslation(glm::vec3 trans);
			void setScale(glm::vec3 scale);
			void setRotationX(float angle);
			void setRotationY(float angle);
			void setRotationZ(float angle);
			void setRotationMatrix(glm::mat4 rotMatrix);
			glm::vec3 getTranslation() { return translation; }
			float getXRotation() { return rotationX; }
			float getYRotation() { return rotationY; }
			float getZRotation() { return rotationZ; }
			glm::mat4 getRotationMatrix() { return rotMatrix; }
			glm::vec3 getScale() { return scale; }
	};
}