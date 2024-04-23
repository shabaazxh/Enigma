#pragma once
#include "../Graphics/Model.h"
#include "../Graphics/Character.h"
#include "../Graphics/Enemy.h"
#include "../Graphics/Player.h"

namespace Enigma
{
	struct World
	{
		//world information for the engine and user to query
		//may require more storage but means searching requires less complexity
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
				float distanceFromPlayer = vec3Length(Enemies[i]->getTranslation() - player->getTranslation());
				if (distanceFromPlayer < 2.f) {
					player->health -= 0.05f;
				}
			}
		}

		void addMeshesToWorld(Player* p, std::vector<Enemy*> enemies) {
			if (!p->noModel) {
				this->Meshes.push_back(p->model);
			}
			for (int i = 0; i < enemies.size(); i++) {
				this->Meshes.push_back(enemies[i]->model);
			}
		}

		void addCharactersToWorld(Player* p, std::vector<Enemy*> enemies) {
			this->Characters.push_back(p);
			for (int i = 0; i < enemies.size(); i++) {
				this->Characters.push_back(enemies[i]);
			}
		}

		void addCharctersToNavmesh(std::vector<Character*> characters) {
			for (int i = 0; i < characters.size(); i++) {
				this->Enemies[0]->addToNavmesh(characters[i], this->Meshes[0]);
			}
		}
	};

}

