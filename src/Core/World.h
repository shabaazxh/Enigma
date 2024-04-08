#pragma once
#include "../Graphics/Model.h"
#include "../Graphics/Character.h"
#include "../Graphics/Enemy.h"
#include "../Graphics/Player.h"

namespace Enigma
{
	struct World
	{
		std::vector<Light> Lights;
		std::vector<Model*> Meshes;
		std::vector<Enemy*> Enemies;
		std::vector<Character*> Characters;
		Player* player;

		void ManageAIs(std::vector<Character*> characters, Model* obj, Player* player, std::vector<Enemy*> Enemies) {
			if (player->moved) {
				for (int i = 0; i < Enemies.size(); i++) {
					Enemies[i]->ManageAI(characters, obj, player);
				}
				player->moved = false;
			}
			for (int i = 0; i < Enemies.size(); i++) {
				Enemies[i]->moveInDirection();
				float distanceFromPlayer = vec3Length(Enemies[i]->translation - player->translation);
				if (distanceFromPlayer < 2.f) {
					player->health -= 1.f;
				}
			}
		}
	};

}

