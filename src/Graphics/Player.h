#pragma once

#include "../Graphics/Character.h"
#include "../Core/Camera.h"

namespace Enigma
{
	class Player : public Character
	{
		public:
			Player(const std::string& filepath, const VulkanContext& context);

			void ManageInputs();
	};
}

