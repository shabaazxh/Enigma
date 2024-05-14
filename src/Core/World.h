#pragma once
#include "../Graphics/Model.h"
#include "../Graphics/Character.h"
#include "../Graphics/Enemy.h"
#include "../Graphics/Player.h"
#include "../Graphics/Light.h"
#include "../Graphics/Common.h"

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

		void ManageAIs(std::vector<Character*> characters, Model* obj, Player* player, std::vector<Enemy*> Enemies, Time* timer) {
			for (int i = 0; i < Enemies.size(); i++) {
				if (Enemies[i]->model->hit) {
					Enemies[i]->health -= 20.f;
					Enemies[i]->model->hit = false;
					if (Enemies[i]->health < 0.f && !Enemies[i]->dead) {
						Enemies[i]->dead = true;
						Enemies[i]->deathTime = timer->current;
					}
				}
				Enemies[i]->ManageAI(characters, obj, player);
			}
			for (int i = 0; i < Enemies.size(); i++) {
				if (Enemies[i]->health < 0.f) {
					if (timer->current - Enemies[i]->deathTime > 10) {
						Enemies[i]->setRotationZ(0);
						Enemies[i]->setTranslation(glm::vec3(60.f, 0.1f, 0.f));
						Enemies[i]->health = 100.f;
						Enemies[i]->dead = false;
					}
					else {
						Enemies[i]->setRotationZ(89);
						Enemies[i]->setTranslation(Enemies[i]->getTranslation());
						glm::mat4 rm = glm::inverse(glm::lookAt(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f) - glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)));
						Enemies[i]->setRotationMatrix(rm);
					}
				}
				else {
					Enemies[i]->moveInDirection();
				}
				float distanceFromPlayer = vec3Length(Enemies[i]->getTranslation() - player->GetPosition());
				if (distanceFromPlayer < 2.f) {
					//player->SetHealth(player->GetPosition() - 0.05);
				}
			}
		}

		void addMeshesToWorld(Player* p, std::vector<Enemy*> enemies) {
			this->Meshes.push_back(p->m_Model);
			for (int i = 0; i < enemies.size(); i++) {
				this->Meshes.push_back(enemies[i]->model);
			}
		}

		void addCharactersToWorld(Player* p, std::vector<Enemy*> enemies) {
			//this->Characters.push_back(p);
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

	inline World WorldInst;
}