#pragma once

#include "../Graphics/Model.h"

namespace Enigma
{
	class Character : public Model
	{
		public:
			float health;
			std::vector<std::string> equipment;

			Character(const std::string& filepath, const VulkanContext& context);

			void ManageAnimation();
			void ManageCollisions();
	};
}