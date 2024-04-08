#pragma once

#include "../Graphics/Character.h"
#include "Player.h"
#include "../Core/Collision.h"
#include <queue>
#include <algorithm>

namespace Enigma
{
	class Enemy : public Character
	{
		public:
			Enemy(const std::string& filepath, const VulkanContext& context, int filetype);

			void ManageAI(std::vector<Character*> characters, Model* obj, Player* player);
			void addToNavmesh(Character* character, Model* obj);
			void moveInDirection();

		private:
			int currentNode;
			std::vector<int> pathToEnemy;
			std::vector<int> findDirection(Player* player);

			void updateNavmesh(std::vector<Character*> character);
			bool isMeshFurtherAway(glm::vec3 dir, glm::vec3 origin, AABB meshAABB);
			bool notVisited(int node, std::vector<int> visited);
	};
}

