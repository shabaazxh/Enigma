#include "Character.h"

namespace Enigma
{
	Character::Character(const std::string& filepath, const VulkanContext& aContext) : Model(filepath, aContext)
	{
		health = 100.f;
	}

	void Character::ManageAnimation() {
		//	ToDo
	}

	void Character::ManageCollisions() {
		//	ToDo
	}
}

