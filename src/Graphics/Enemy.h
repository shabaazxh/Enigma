#pragma once

#include "../Graphics/Character.h"
#include "Player.h"
#include "../Core/Collision.h"
#include <queue>

namespace Enigma
{
	class Enemy : public Character
	{
		public:
			Enemy(const std::string& filepath, const VulkanContext& context, int filetype);

			void ManageAI(Character* character, Model* obj);

		private:
			void addToNavmesh(Character* character, Model* obj);
			glm::vec3 findDirection();
			void moveInDirection(glm::vec3 dir);
			bool isMeshFurtherAway(glm::vec3 dir, glm::vec3 origin, AABB meshAABB);
			bool notVisited(int node, std::vector<int> visited);
	};
}

