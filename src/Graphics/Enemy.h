#pragma once

#include "../Graphics/Character.h"

namespace Enigma
{
	class Enemy : public Character
	{
		public:
			Enemy(const std::string& filepath, const VulkanContext& context, int filetype);

			void ManageAI();
	};
}

