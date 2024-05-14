#pragma once

#include "../Graphics/Character.h"
#include "Player.h"
#include "../Core/Collision.h"
#include <queue>
#include <algorithm>
#include "../Graphics/Common.h"

namespace Enigma
{
	class Enemy : public Character
	{
	public:
		Enemy(const std::string& filepath, const VulkanContext& context, int filetype);
		Enemy(const std::string& filepath, const VulkanContext& context, int filetype, glm::vec3 trans, glm::vec3 scale);
		Enemy(const std::string& filepath, const VulkanContext& context, int filetype, glm::vec3 trans, glm::vec3 scale, float x, float y, float z);
		Enemy(const std::string& filepath, const VulkanContext& context, int filetype, glm::vec3 trans, glm::vec3 scale, glm::mat4 rm);

		void ManageAI(std::vector<Character*> characters, Model* obj, Player* player);
		void addPlayerToNavmesh(Player* player, Model* obj);
		void addToNavmesh(Character* character, Model* obj);
		void moveInDirection();

		double deathTime;
		bool dead = false;

	private:
		int currentNode;
		std::vector<int> pathToEnemy;
		std::vector<int> findDirection(Player* player);

		void updateNavmesh(std::vector<Character*> character);
		bool notVisited(int node, std::vector<int> visited);
	};
}

