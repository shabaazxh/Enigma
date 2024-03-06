#pragma once

#include "../Graphics/Character.h"

namespace Enigma
{
	class Player : public Character
	{
		public:
			Player(const std::string& filepath, Allocator& aAllocator, const VulkanContext& context);

			void ManageInputs();
	};
}

