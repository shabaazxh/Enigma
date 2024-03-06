#include "Character.h"

namespace Enigma
{
	Character::Character(const std::string& filepath, Allocator& aAllocator, const VulkanContext& aContext) : Model(filepath, aAllocator, aContext)
	{
		health = 100.f;
		LoadModel(filepath);
		GetBoundingBox();
	}

	void Character::ManageAnimation() {
		//	ToDo
	}

	void Character::ManageCollisions() {
		//	ToDo
	}
}

