#pragma once
#ifndef PLAYER_H
#define PLAYER_H

#include "../Graphics/Character.h"

class Camera;

namespace Enigma
{
	class Player : public Character
	{
		public:
			Player(const std::string& filepath, const VulkanContext& context, int filetype);
			Player(const VulkanContext& context);

			void ManageInputs();
	};
}

#endif // PLAYER_H

