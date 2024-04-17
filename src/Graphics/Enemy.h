#pragma once

#include "../Graphics/Character.h"
#include "Player.h"
#include "../Core/Collision.h"
#include <queue>
#include <algorithm>
#include "Common.h" // should forward declare the properties for path finding you defined in here, then include in .cpp

namespace Enigma
{
	class Enemy : public Character
	{
		public:
			Enemy(const std::string& filepath, const VulkanContext& context, int filetype);

			void ManageAI(std::vector<Character*> character, Model* obj, Player* player);

			void addToNavmesh(Character* character, Model* obj);

		private:
			int currentNode;
			std::vector<int> pathToEnemy;
			std::vector<int> findDirection();

			void updateNavmesh(std::vector<Character*> character);
			void moveInDirection();
			bool isMeshFurtherAway(glm::vec3 dir, glm::vec3 origin, AABB meshAABB);
			bool notVisited(int node, std::vector<int> visited);
	};
}

