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
	};

	inline World WorldInst;
}